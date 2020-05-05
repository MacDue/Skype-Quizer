#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <fstream>
#include <utility>
#include <iostream>
#include <filesystem>
#include <unordered_map>

#include <CImg.h>
#include <boost/algorithm/string.hpp>

#include "crow.h"
#include "v4l2_cimg.h"
#include "imagehelper.h"
#include "json_wrapper.h"
#include "rapidjson/istreamwrapper.h"

using namespace DueUtil::Images;

constexpr char const * VIDEO_OUT = "/dev/video0";

constexpr int WIDTH = 640;
constexpr int HEIGHT = 480;

#define ASSETS_BASE "./assets"
#define TITLE "Ben's Skype Quiz!"

namespace json = SleepyDiscord::json;

rapidjson::Document load_json_file(std::string path) {
  std::ifstream ifs(path);
  rapidjson::IStreamWrapper isw(ifs);
  rapidjson::Document d;
  d.ParseStream(isw);
  return d;
}

#define JSONStructCtor(struct_name)                      \
  struct_name() = default;                               \
  struct_name(const json::Value & json)                  \
    : struct_name(json::fromJSON<struct_name>(json)) {}  \
  struct_name(const std::string_view & json)             \
    : struct_name(json::fromJSON<struct_name>(json)) {}

struct Question {
  JSONStructCtor(Question)
  std::string question;
  std::string answer;

  JSONStructStart
    std::make_tuple(
      json::pair(&Question::question, "Quiz question", json::REQUIRIED_FIELD),
      json::pair(&Question::answer  , "Answer"       , json::REQUIRIED_FIELD)
    );
  JSONStructEnd
};

using Categories = std::vector<std::pair<std::string, std::vector<Question>>>;

Categories const & load_categories() {
  static Categories categories;
  for (auto const & entry : std::filesystem::directory_iterator(ASSETS_BASE "/questions")) {
    if (entry.path().extension() == ".json") {
      auto category_json = load_json_file(entry.path());
      auto category_name = entry.path().stem().string();
      boost::replace_all(category_name, "\"", "");
      categories.push_back(std::make_pair(category_name, std::vector<Question>{}));
      auto& [_, questions] = categories.back();
      std::cout << "Loading " << category_name << " questions!\n";
      for (Question question : category_json.GetArray()) {
        std::cout << '\t' << question.question << '\n';
        questions.push_back(question);
      }
      std::cout << '\n';

      std::random_shuffle(questions.begin(), questions.end());
    }
  }

  std::random_shuffle(categories.begin(), categories.end());
  return categories;
}

std::vector<std::string> wrap_text(
  std::string text, DueFont & font, int font_size, int max_width
) {
  std::stringstream ss(text);

  int current_line_len = 0;
  std::stringstream current_line;

  std::vector<std::string> lines;
  std::string word;

  bool first_word = true;
  while(ss >> word) {
    int word_width = text_width(word, font, font_size);
    if (current_line_len + word_width > max_width) {
      lines.push_back(current_line.str());
      current_line.str("");
      first_word = true;
      current_line_len = 0;
    }

    auto padding = !first_word ? " " : "";
    current_line << padding << word;
    first_word = false;
    current_line_len += word_width + text_width(padding, font, font_size);
  }

  if (current_line_len > 0) {
    lines.push_back(current_line.str());
  }

  return lines;
}

void draw_centred_wrapped_text(
  CImg<uint8_t>& canvas,
  std::string text, DueFont& font, int font_size, int max_width,
  Colour const & colour = DUE_BLACK,
  Colour const & stroke = DUE_WHITE
) {
  auto lines = wrap_text(text, font, font_size, max_width);
  int height = lines.size() * font_size;

  for (int i = 0; i < lines.size(); i++) {
    auto const & line = lines[i];
    int width = text_width(line, font, font_size);
    draw_text(canvas,
      (canvas.width() - width)/2,
      (canvas.height() - height)/2 + i * font_size,
      line, font, font_size, colour, -1, 1, stroke);
  }
}

constexpr int CATEGORIES_TO_PLAY = 5;
constexpr int QUESTIONS_PER_CATEGORY = 4;

#define TITLE_SIZE 48
#define TEXT_SIZE 32

class Quiz final {
  CImg<uint8_t> canvas = CImg<uint8_t>(WIDTH, HEIGHT, 1, 3);
  v4l2_cimg     v4l2   = v4l2_cimg(VIDEO_OUT, canvas.width(), canvas.height());
  DueFont       font   = DueFont(ASSETS_BASE "/robo.ttf");

  Categories const & categories = load_categories();

  std::atomic<bool> display_ready = true;

  // quiz state FSM
  enum {
    QUIZ_TITLE,
    SHOWING_CATEGORY,
    SHOWING_QUESTION,
    SHOWING_ANSWER,
    ANSWERS_TITLE
  } current_state {QUIZ_TITLE};

  int current_question = 0, current_category = 0;

  void draw_title_page(std::string const & title) {
    int width = text_width(title, font, 32);
    draw_centred_wrapped_text(
      canvas, title, font, TITLE_SIZE, WIDTH * 0.8,
      Colour(0, 175, 240));
  }

  void draw_question_page() {
    auto const & [_, questions] = categories[current_category];

    std::string const & question = questions[current_question].question;
    draw_centred_wrapped_text(
      canvas, question, font, TEXT_SIZE, WIDTH * 0.8,
      Colour(255, 123, 0));
  }

  void draw_answer_page() {
    auto const & [_, questions] = categories[current_category];

    std::string const & answer = questions[current_question].answer;
    draw_centred_wrapped_text(
      canvas, answer, font, TEXT_SIZE, WIDTH * 0.8,
      Colour(255, 102, 255));
  }

  public:
    Quiz() {
      render();
    }

    void next_page() {
      switch (current_state)
      {
      case QUIZ_TITLE:
        current_state = SHOWING_CATEGORY;
        break;
      case SHOWING_CATEGORY:
        current_question = 0;
        current_state = SHOWING_QUESTION;
        break;
      case SHOWING_ANSWER:
      case SHOWING_QUESTION:
        current_question += 1;
        if (current_question >= QUESTIONS_PER_CATEGORY
          || current_question >= std::get<1>(categories[current_category]).size()
        ) {
          if (current_state == SHOWING_QUESTION) {
            current_question = 0;
            current_state = ANSWERS_TITLE;
          } else {
            current_state = SHOWING_CATEGORY;
            if (current_category < CATEGORIES_TO_PLAY) {
              current_category += 1;
            } else {
              current_category = 0;
            }
          }
        }
        break;
      case ANSWERS_TITLE:
        current_state = SHOWING_ANSWER;
      }
      render();
    }
    // void prev_page() { --this->current_question; render(); }
    // void goto_page(int page) { this->current_question = page; render(); }

    void render() {
      display_ready = false;
      canvas.draw_plasma();
      canvas.draw_rectangle(0, 0, WIDTH, HEIGHT, DUE_BLACK.c_arr(), 0.9);

      switch (current_state)
      {
      case QUIZ_TITLE:
        draw_title_page(TITLE);
        break;
      case SHOWING_CATEGORY: {
        auto const & [category, _] = categories[current_category];
        draw_title_page(category);
        break;
      }
      case SHOWING_QUESTION:
        draw_question_page();
        break;
      case ANSWERS_TITLE:
        draw_title_page("Answers");
        break;
      case SHOWING_ANSWER:
        draw_answer_page();
        break;
      }

      display_ready = true;
    }

    void display() {
      while (!display_ready);
      v4l2.display(canvas);
    }
};


int main() {
  using namespace std::chrono_literals;

  Quiz quiz;

  std::thread controller_thread([&]{
    crow::SimpleApp controller;

    CROW_ROUTE(controller, "/next")([&](){
      quiz.next_page();
      return crow::response(200);
    });

    // CROW_ROUTE(controller, "/prev")([&](){
    //   quiz.prev_page();
    //   return crow::response(200);
    // });

    controller.port(5000).run();
  });

  while (true) {
    quiz.display();
    std::this_thread::sleep_for(1s/24);
  }

  controller_thread.join();
}


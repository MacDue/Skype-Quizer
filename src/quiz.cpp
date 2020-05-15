#include <atomic>
#include <thread>
#include <random>
#include <chrono>
#include <string>
#include <vector>
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

#define VIDEO_OUT "/dev/video0"

// This must match what VIDEO_OUT expects!
#define WIDTH 640
#define HEIGHT 480
#define SAFE_WIDTH (WIDTH * 0.8)

#define ASSETS_BASE "./assets"
#define TITLE "Ben's Skype Quiz!"

#define TITLE_SIZE 48
#define TEXT_SIZE 32

namespace json = SleepyDiscord::json;

rapidjson::Document load_json_file(std::string path) {
  std::ifstream ifs(path);
  rapidjson::IStreamWrapper isw(ifs);
  rapidjson::Document d;
  d.ParseStream(isw);
  return d;
}

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

Categories & load_categories() {
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
    }
  }
  return categories;
}

std::vector<std::string> wrap_text(
  std::string text, DueFont & font, int font_size, int max_width
) {
  std::stringstream words(text), current_line;
  int current_line_len = 0;

  std::vector<std::string> lines;
  std::string word;

  bool first_word = true;
  while(words >> word) {
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

int draw_centred_wrapped_text(
  CImg<uint8_t>& canvas,
  std::string text, DueFont& font, int font_size, int max_width,
  Colour const & colour = DUE_BLACK,
  int start_y = -1,
  Colour const & stroke = DUE_WHITE
) {
  auto lines = wrap_text(text, font, font_size, max_width);
  int height = lines.size() * font_size;

  start_y = start_y < 0 ? (canvas.height() - height)/2 : start_y;
  for (int i = 0; i < lines.size(); i++) {
    auto const & line = lines[i];
    int width = text_width(line, font, font_size);
    draw_text(canvas,
      (canvas.width() - width)/2,
      start_y + i * font_size,
      line, font, font_size, colour, -1, 1, stroke);
  }

  return start_y + height;
}

Colour const SKYPE_BLUE       (0,   175, 240);
Colour const QUESTION_ORANGE  (255, 123, 0  );
Colour const ANSWER_PINK      (255, 102, 255);

class Quiz final {
  CImg<uint8_t> canvas = CImg<uint8_t>(WIDTH, HEIGHT, 1, 3);
  v4l2_cimg     v4l2   = v4l2_cimg(VIDEO_OUT, canvas.width(), canvas.height());
  DueFont       font   = DueFont(ASSETS_BASE "/robo.ttf");

  Categories & categories = load_categories();

  std::atomic<bool> display_ready = true;

  // Quiz state (FSM)
  int categories_to_play, questions_per_category;

  enum {
    QUIZ_TITLE,
    SHOWING_CATEGORY,
    SHOWING_QUESTION,
    SHOWING_ANSWER,
    ANSWERS_TITLE
  } current_state {QUIZ_TITLE};

  int current_question = 0, current_category = 0;

  void draw_title_page(std::string title) {
    int width = text_width(title, font, 32);
    draw_centred_wrapped_text(canvas,
      title, font, TITLE_SIZE, SAFE_WIDTH, SKYPE_BLUE);
  }

  void draw_question_page() {
    auto const & [_, questions] = categories[current_category];

    std::string const & question = questions[current_question].question;
    draw_centred_wrapped_text(canvas,
      question, font, TEXT_SIZE, SAFE_WIDTH, QUESTION_ORANGE);
  }

  void draw_answer_page() {
    auto const & [_, questions] = categories[current_category];
    Question const & question = questions[current_question];

    // Wasted computation but fine for this!
    int question_size = TEXT_SIZE * 0.7;
    int height =
      wrap_text(question.question, font, question_size, SAFE_WIDTH).size() * question_size
      + wrap_text(question.answer, font, TEXT_SIZE, SAFE_WIDTH).size() * TEXT_SIZE;

    int offset = draw_centred_wrapped_text(canvas,
      question.question, font, question_size, SAFE_WIDTH,
      QUESTION_ORANGE, (HEIGHT - height)/2);

    draw_centred_wrapped_text(canvas,
      question.answer, font, TEXT_SIZE, SAFE_WIDTH, ANSWER_PINK, offset);
  }

  public:
    Quiz() {
      reset(5, 10);
      render();
    }

    void next_page() {
      switch (current_state) {
        case QUIZ_TITLE:
          current_state = SHOWING_CATEGORY;
          break;
        case SHOWING_CATEGORY:
          current_question = 0;
          current_state = SHOWING_QUESTION;
          break;
        case SHOWING_ANSWER:
        case SHOWING_QUESTION:
        {
          auto& questions = std::get<1>(categories[current_category]);
          current_question += 1;
          if (current_question >= questions_per_category
            || current_question >= questions.size()
          ) {
            if (current_state == SHOWING_QUESTION) {
              current_question = 0;
              current_state = ANSWERS_TITLE;
            } else {
              // Simple hack to remove repeat questions
              if (current_question < questions.size()) {
                questions.erase(questions.begin(), questions.begin() + current_question);
                current_category += 1;
              } else {
                categories.erase(categories.begin() + current_category);
                categories_to_play -= 1;
              }
              current_state = SHOWING_CATEGORY;
              if (current_category >= categories_to_play
                || current_category >= categories.size()
              ) {
                current_state = QUIZ_TITLE;
                current_category = 0;
              }
            }
          }
          break;
        }
        case ANSWERS_TITLE:
          current_state = SHOWING_ANSWER;
      }
      render();
    }

    void reset(int categories_to_play, int questions_per_category) {
      this->categories_to_play = categories_to_play;
      this->questions_per_category = questions_per_category;

      static std::random_device rand;
      static std::mt19937 prng(rand());
      std::shuffle(categories.begin(), categories.end(), prng);
      for (auto& [_, questions]: categories) {
        std::shuffle(questions.begin(), questions.end(), prng);
      }

      current_state = QUIZ_TITLE;
      current_category = 0;
      current_question = 0;
      render();
    }

    void render() {
      display_ready = false;
      canvas.draw_plasma();
      canvas.draw_rectangle(0, 0, WIDTH, HEIGHT, DUE_BLACK.c_arr(), 0.9);

      switch (current_state) {
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

    CROW_ROUTE(controller,"/reset/<int>/<int>")
    ([&](int categories_to_play, int questions_per_category){
      quiz.reset(categories_to_play, questions_per_category);
      return crow::response(200);
    });

    controller.port(5000).run();
  });

  // This wastes a lot of CPU, but if the frame is not refreshed enough
  // skype thinks the "webcam" has died!
  while (true) {
    quiz.display();
    std::this_thread::sleep_for(1s/24);
  }

  controller_thread.join();
}

import json
import time
import requests
from pathlib import Path

from tqdm import tqdm
from bs4 import BeautifulSoup

QUESTIONS_DIR = "./questions"
QUIZ_ZONE = "https://www.quiz-zone.co.uk/"
HEADERS = { 'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.95 Safari/537.36'}

def get_categories():
  r = requests.get(f'{QUIZ_ZONE}/questionsbycategory.html', headers=HEADERS)
  if (r.status_code != 200):
    raise ConnectionError(f'Failed to fetch categories {r.status_code}')
  soup = BeautifulSoup(r.text, features="html.parser")
  links = soup.find_all('a', href=True)

  categories = {}
  for link in links:
    if link['href'].startswith('questionsbycategory/'):
      name, count = link.contents[0].split('(')
      count = int(count.replace(')', '').replace('questions', '').strip())
      categories[name.strip()] = (link['href'].split('/')[1], count)
  return categories

def get_questions(questions, page=0):
  r = requests.get(
    f'{QUIZ_ZONE}/questionsbycategory/{questions}/{page}/answers.html',
    headers=HEADERS)
  if (r.status_code != 200):
    raise ConnectionError(f'Failed to fetch questions {r.status_code}')
  soup = BeautifulSoup(r.text, features="html.parser")
  questions = soup.find('table').find_all('tr')
  questions = [
    {
      'Quiz question': ' '.join(q.find('span').contents).replace('`', '\''),
      'Answer': ' '.join(q.find('b').contents).replace('`', '\'')
    }
    for q in questions[1:]
  ]
  return questions

Path(QUESTIONS_DIR).mkdir(parents=True, exist_ok=True)
for category, (key, count) in get_categories().items():
  questions = []
  print(f'Fetching "{category}" questions!')
  for page in tqdm(range(count//20)):
    try:
      questions.extend(get_questions(key, page * 20))
    except Exception as e:
      print('Stopping fetching questsions...', e)
    time.sleep(1)
  out_file = QUESTIONS_DIR + f'/{category}.json'
  with open(out_file, 'w') as f:
    json.dump(questions, f, ensure_ascii=False, indent=4)
  print(f'Wrote: {out_file}')

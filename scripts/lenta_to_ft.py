import argparse
import csv
import random

def main(input_file, output_file, part):
    with open(input_file, "r") as r:
        next(r)
        reader = csv.reader(r, delimiter=',')
        records = []
        for row in reader:
            url, title, text, topic, tag = row
            topic = topic.strip()
            tag = tag.strip()
            true_topic = None
            if topic == "Экономика":
                true_topic = "economy"
            elif topic == "Спорт":
                true_topic = "sports"
            elif topic == "Культура":
                true_topic = "entertainment"
            elif topic == "Наука и техника" and tag == "Игры":
                true_topic = "entertainment"
            elif topic == "Наука и техника" and (tag == "Наука" or tag == "Космос" or tag == "Жизнь"):
                true_topic = "science"
            elif topic == "Мир" and (tag == "Политика" or tag == "Происшествия" or tag == "Общество" or tag == "Конфликты" or tag == "Преступность"):
                true_topic = "society"
            elif topic == "Россия" and (tag == "Политика" or tag == "Общество" or tag == "Происшествия"):
                true_topic = "society"
            elif topic == "Наука и техника" and tag == "Оружие":
                true_topic = "society"
            elif topic == "Наука и техника" and (tag == "Гаджеты" or tag == "Софт" or tag == "Техника"):
                true_topic = "technology"
            elif topic == "Силовые структуры":
                true_topic = "society"
            elif topic == "Интернет и СМИ" and (tag == "Мемы" or tag == "Киберпреступность" or tag == "Интернет" or tag == "Вирусные ролики"):
                true_topic = "technology"
            elif topic == "Ценности" and tag == "Стиль":
                true_topic = "other"
            elif topic == "Бывший СССР":
                continue
            elif topic == "Бизнес":
                true_topic = "economy"
            elif topic == "Культпросвет":
                true_topic = "entertainment"
            elif topic == "Наука и техника" and tag == "История":
                true_topic = "science"
            elif topic == "Из жизни" and tag == "Происшествия":
                true_topic = "society"
            elif topic == "Ценности" and tag == "Движение":
                true_topic = "technology"
            elif topic == "Ценности" and (tag == "Явления" or tag == "Внешний вид"):
                true_topic = "other"
            elif topic == "Путешествия" and tag == "Происшествия":
                true_topic = "society"
            elif topic == "Из жизни":
                continue
            elif topic == "Интернет и СМИ":
                continue
            elif topic == "Ценности":
                continue
            elif topic == "Россия":
                continue
            elif topic == "Дом":
                continue
            else:
                continue
            records.append({"url": url, "title": title, "text": text, "res": true_topic})
        print(len(records))
        with open(output_file, "w") as w:
            for r in records:
                if random.random() > part:
                    continue
                w.write("__label__{} {} {}\n".format(r["res"], r["title"], r["text"]))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-file", type=str, required=True)
    parser.add_argument("--output-file", type=str, required=True)
    parser.add_argument("--part", type=int, default=0.2)
    args = parser.parse_args()
    main(**vars(args))


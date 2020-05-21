import argparse
import json
import pyonmttok
from collections import Counter
from collections import defaultdict
from sklearn.metrics import cohen_kappa_score


def clean_text(text):
    return text.replace("\t", " ").replace("\n", " ").replace('"', '')


def preprocess(text, tokenizer):
    text = str(text).strip().replace("\n", " ").replace("\xa0", " ").lower()
    tokens, _ = tokenizer.tokenize(text)
    text = " ".join(tokens)
    return text


def main(answers_file_name, original_json, honey_output_file_name, ft_output_file_name, min_votes, target_file_name):
    with open(answers_file_name, "r") as r:
        header = tuple(next(r).strip().split("\t"))
        records = []
        for line in r:
            fields = line.strip().split("\t")
            assert len(fields) == len(header), fields
            records.append(dict(zip(header, fields)))
    # Filter honeypots out
    records = [r for r in records if not r["GOLDEN:res"]]

    # Normalize fields
    for r in records:
        r.pop("GOLDEN:res", None)
        r.pop("HINT:text", None)
        for key, value in r.items():
            new_key = key.split(":")[-1]
            r[new_key] = r.pop(key)

    if original_json:
        with open(original_json, "r") as r:
            data = json.load(r)
            title2url = {clean_text(d["title"]): d["url"] for d in data}
            for r in records:
                r["url"] = title2url[clean_text(r["title"])]


    # Calc inter-annotator agreement
    annotator2labels = defaultdict(dict)
    unique_keys = list(set([r["url"] for r in records]))
    unique_workers = list(set([r["worker_id"] for r in records]))
    unique_res = list(set([r["res"] for r in records]))
    res2num = {res: i for i, res in enumerate(unique_res)}
    for r in records:
        annotator2labels[r["worker_id"]][r["url"]] = r["res"]
    worker2labels = {}
    for worker_id in unique_workers:
        worker_labels = []
        worker_res = annotator2labels[worker_id]
        for key in unique_keys:
            if key not in worker_res:
                worker_labels.append(-1)
                continue
            worker_labels.append(res2num[worker_res[key]])
        worker2labels[worker_id] = worker_labels
    scores = []
    for w1, labels1 in worker2labels.items():
        for w2, labels2 in worker2labels.items():
            if w1 == w2:
                continue
            fixed_labels1 = []
            fixed_labels2 = []
            for l1, l2 in zip(labels1, labels2):
                if l1 == -1 or l2 == -1:
                    continue
                fixed_labels1.append(l1)
                fixed_labels2.append(l2)
            if not fixed_labels1 or not fixed_labels2:
                print("{} vs {}: no intersection".format(w1, w2))
            else:
                score = cohen_kappa_score(fixed_labels1, fixed_labels2)
                if -1.0 <= score <= 1.0:
                    scores.append(score)
                print("{} vs {}: {}".format(w1, w2, score))
    print("Avg kappa score: {}".format(sum(scores)/len(scores)))

    results = defaultdict(list)
    for r in records:
        results[r["url"]].append(r["res"])

    data = {r["url"]: r for r in records}
    votes_count = Counter()
    print("Bad examples: ")
    for url, res in results.items():
        res_count = Counter(res)
        votes_for_win = res_count.most_common(1)[0][1]
        votes_count[votes_for_win] += 1
        if votes_for_win < min_votes:
            print("URL:", url)
            print("Answers:", ", ".join(res))
            data.pop(url)
        else:
            data[url]["res"] = res_count.most_common(1)[0][0]
    print("Votes for majority: ")
    for votes, sample_count in votes_count.items():
        print("{}: {}".format(votes, sample_count))

    rub_cnt = Counter()
    for _, d in data.items():
        rub_cnt[d["res"]] += 1
    print("Rubrics: ")
    for rub, cnt in rub_cnt.most_common():
        print("{}: {}".format(rub, cnt))

    if honey_output_file_name:
        with open(honey_output_file_name, "w") as w:
            w.write("{}\t{}\t{}\t{}\n".format("INPUT:url", "INPUT:title", "INPUT:text", "GOLDEN:res"))
            for d in data.values():
                w.write("{}\t{}\t{}\t{}\n".format(d["url"], d["title"], d["text"], d["res"]))

    if ft_output_file_name:
        tokenizer = pyonmttok.Tokenizer("conservative")
        with open(ft_output_file_name, "w") as w:
            for d in data.values():
                text = preprocess(d["text"], tokenizer)
                title = preprocess(d["title"], tokenizer)
                w.write("__label__{} {} {}\n".format(d["res"], title, text))

    if target_file_name:
        with open(target_file_name, "w") as w:
            records = [{
                "url": d["url"],
                "title": d["title"],
                "text": d["text"],
                "category": d["res"]
            } for d in data.values()]
            json.dump(records, w, ensure_ascii=False, indent=4, sort_keys=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--answers-file-name", type=str, required=True)
    parser.add_argument("--original-json", type=str, default=None)
    parser.add_argument("--honey-output-file-name", type=str, default=None)
    parser.add_argument("--ft-output-file-name", type=str, default=None)
    parser.add_argument("--target-file-name", type=str, default=None)
    parser.add_argument("--min-votes", type=int, default=3)
    args = parser.parse_args()
    main(**vars(args))


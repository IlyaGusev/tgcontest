import sys
import json
import math
from collections import Counter
import pyonmttok
tokenizer = pyonmttok.Tokenizer("conservative", joiner_annotate=False)

def preprocess(text):
    text = str(text).strip().replace("\n", " ").replace("\xa0", " ").lower()
    tokens, _ = tokenizer.tokenize(text)
    text = " ".join(tokens)
    return text

def read_jsonl_contents(file_name):
    with open(file_name, "r") as r:
        for line in r:
            record = json.loads(line)
            title = preprocess(record["title"])
            text = preprocess(record["text"])
            content = " ".join((title.strip(), text.strip()))
            yield content

def calc_idfs(contents, vocab_size=10000, remove_first=1000):
    doc_count = 0
    words_counts = Counter()
    words_idfs = dict()
    for content in contents:
        words = set(content.split(" "))
        words_counts.update(list(words))
        doc_count += 1
    doc_count = float(doc_count)
    for word, word_count in words_counts.most_common(vocab_size + remove_first)[remove_first:]:
        idf = math.log((doc_count + 1)/(word_count + 1))
        words_idfs[word] = idf
    return words_idfs

words_idfs = calc_idfs(read_jsonl_contents(sys.argv[1]))
with open(sys.argv[2], "w") as w:
    w.write("<unk>\t0.0\n")
    for word, idf in words_idfs.items():
        w.write("{}\t{:.4f}\n".format(word, idf))

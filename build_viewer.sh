TEMPLATES_DIR="$1"
OUTPUT_DIR="$2"
DEMO_DATE="$3"
DEMO_DATA="$4"
RU_CAT_DATA="$5"
RU_CAT_GOLD="$6"
EN_CAT_DATA="$7"
EN_CAT_GOLD="$8"
RU_THREADS_DATA="$9"
RU_THREADS_GOLD="${10}"

DEMO_TOPS=$(mktemp)
RU_CAT_METRICS=$(mktemp)
EN_CAT_METRICS=$(mktemp)
RU_CAT_OUTPUT=$(mktemp)
EN_CAT_OUTPUT=$(mktemp)
RU_THREADS_OUTPUT=$(mktemp)
RU_THREADS_METRICS=$(mktemp)
VERSION=$(git log --pretty=format:'%h' -n 1)

./build/tgnews top ${DEMO_DATA} --print_top_debug_info > ${DEMO_TOPS}
./build/tgnews categories ${RU_CAT_DATA} --languages ru --save_not_news > ${RU_CAT_OUTPUT}
./build/tgnews categories ${EN_CAT_DATA} --languages en --save_not_news > ${EN_CAT_OUTPUT}
./build/tgnews threads ${RU_THREADS_DATA} --languages ru > ${RU_THREADS_OUTPUT}

python3.6 viewer/convert.py \
    --documents-file ${DEMO_DATA} \
    --tops-file ${DEMO_TOPS} \
    --templates-dir ${TEMPLATES_DIR} \
    --output-dir ${OUTPUT_DIR} \
    --version "${VERSION}" \
    --date "${DEMO_DATE}"

python3.6 viewer/calc_categories_metrics.py \
    --documents-file ${RU_CAT_DATA} \
    --cat-gold ${RU_CAT_GOLD} \
    --cat-output ${RU_CAT_OUTPUT} \
    --output-json ${RU_CAT_METRICS}

python3.6 viewer/calc_categories_metrics.py \
    --documents-file ${EN_CAT_DATA} \
    --cat-gold ${EN_CAT_GOLD} \
    --cat-output ${EN_CAT_OUTPUT} \
    --output-json ${EN_CAT_METRICS}

python3.6 viewer/calc_threads_metrics.py \
    --original-jsonl "${RU_THREADS_DATA}" \
    --threads-json "${RU_THREADS_OUTPUT}" \
    --clustering-markup "${RU_THREADS_GOLD}" \
    --output-json "${RU_THREADS_METRICS}"

python3.6 viewer/metrics_to_html.py \
    --categories-json "${RU_CAT_METRICS}" \
    --threads-json "${RU_THREADS_METRICS}" \
    --templates-dir "${TEMPLATES_DIR}" \
    --output-dir "${OUTPUT_DIR}/ru" \
    --language ru \
    --version "${VERSION}" \
    --date "${RU_CAT_GOLD}"

python3.6 viewer/metrics_to_html.py \
    --categories-json "${EN_CAT_METRICS}" \
    --templates-dir "${TEMPLATES_DIR}" \
    --output-dir "${OUTPUT_DIR}/en" \
    --language en \
    --version "${VERSION}" \
    --date "${EN_CAT_GOLD}"


# Social Sentiment Pipeline (YouTube → C++ → Python → Power BI)

End-to-end data pipeline to collect and analyze **YouTube comments** about a brand (e.g., Verizon).  
Built with **C++ for ingestion**, **Python for transformation + sentiment scoring**, and **Power BI for dashboarding**.  

This project demonstrates real-world **ETL (Extract, Transform, Load)** skills with meaningful business insights.

---

## 🚀 Architecture
YouTube API → C++ CLI (ingest_yt) → NDJSON (data/raw)
→ Python (transform_comments.py) → CSV/Parquet (data/curated)
→ Power BI report (pbix)

yaml
Copy code

---

## 🛠 Tech Stack
- **C++20**: ETL ingestion (CMake, vcpkg, libcurl, nlohmann/json, spdlog)  
- **Python**: Data cleaning & sentiment analysis (pandas, vaderSentiment, pyarrow)  
- **Power BI**: Interactive dashboard (KPIs, gauges, trends, word cloud)  

---

## 📂 Folder Structure
cpp_ingest/ # C++ ETL client
include/, src/
build/ # compiled binaries (ignored in git)

py_transform/ # Python transformer
requirements.txt
transform_comments.py

data/
raw/ # NDJSON outputs from ingestion (ignored in git)
curated/ # CSV/Parquet cleaned outputs (ignored in git)

run_verizon.sh # Batch ingestion script for multiple keyword sets
README.md

yaml
Copy code

---

## 🔧 Setup Instructions

### 1. Build C++ Ingestion
From Git Bash with MSVC + vcpkg installed:
```bash
cd cpp_ingest
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
This produces:

bash
Copy code
cpp_ingest/build/Release/ingest_yt.exe
2. Run Ingestion
Set your YouTube Data API key:

bash
Copy code
export YOUTUBE_API_KEY="YOUR_API_KEY"
Example run:

bash
Copy code
./ingest_yt.exe --brand "verizon" --keywords "verizon,5g" --days 30 --limit_videos 30 --out ../../data/raw/verizon_5g.ndjson
Batch run across multiple keywords:

bash
Copy code
cd cpp_ingest/build
bash run_verizon.sh
3. Transform + Sentiment (Python)
Install dependencies:

bash
Copy code
cd py_transform
pip install -r requirements.txt
Run the transformer:

bash
Copy code
python transform_comments.py --in ../data/raw --out ../data/curated --brand verizon
Outputs:

comments_clean.csv → full dataset with sentiment labels

comments_clean.parquet → fast format (optional)

sentiment_counts.csv → aggregated positive/negative/neutral counts

4. Power BI Dashboard
Open Power BI Desktop → Get Data → CSV and load:

data/curated/comments_clean.csv

data/curated/sentiment_counts.csv

Suggested visuals:

Card: Total number of comments

Line Chart: Sentiment over time (x=created_dt, legend=sent_label)

Stacked Bar: Top keywords by sentiment (keywords_str)

Gauges: Positive % / Negative % sentiment

Pie Chart: Overall sentiment distribution

Table: Most liked comments (author, text, like_count)

(Optional) Word Cloud: Common terms in comments
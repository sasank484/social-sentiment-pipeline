markdown
Copy code
# Power BI Dashboard Guide

This guide builds a Power BI dashboard from `storage/sentiment.db` (via CSV export).

## Import CSV
1. After you run the ETL and load SQLite, export to CSV:
   ```bash
   python py_transform/export_csv.py --db storage/sentiment.db --out bi/powerbi/posts_latest.csv
Power BI Desktop → Get data → Text/CSV → select bi/powerbi/posts_latest.csv.

In Power Query, set created_utc to Date/Time (UTC). Close & Apply.

Fields (single table)
post_id (Text)

source (Text)

brand (Text)

created_utc (Date/Time)

text (Text)

sentiment_score (Decimal)

sentiment_label (Text: "neg" | "neu" | "pos")

like_count (Whole number)

video_id (Text)

url (Text)

DAX Measures
DAX
Copy code
Total Comments = COUNTROWS(Posts)
Avg Sentiment = AVERAGE(Posts[sentiment_score])
Negative Comments = CALCULATE(COUNTROWS(Posts), Posts[sentiment_label] = "neg")
Positive Comments = CALCULATE(COUNTROWS(Posts), Posts[sentiment_label] = "pos")
Neutral Comments = CALCULATE(COUNTROWS(Posts), Posts[sentiment_label] = "neu")

Negative % =
VAR total = [Total Comments]
RETURN DIVIDE([Negative Comments], total, 0)

Positive % =
VAR total = [Total Comments]
RETURN DIVIDE([Positive Comments], total, 0)

Neutral % =
VAR total = [Total Comments]
RETURN DIVIDE([Neutral Comments], total, 0)
Recommended visuals
KPI Cards: Total Comments, Avg Sentiment, Negative %

Line chart: Axis = created_utc, Value = Avg Sentiment

Column chart: Axis = sentiment_label, Value = Total Comments

Table: created_utc, sentiment_label, sentiment_score, text, url

Slicers: Date range (created_utc), Brand, Source

Refresh
Re-run ETL to update SQLite.

Export fresh CSV:

bash
Copy code
python py_transform/export_csv.py --db storage/sentiment.db --out bi/powerbi/posts_latest.csv
In Power BI, Refresh.

python
Copy code


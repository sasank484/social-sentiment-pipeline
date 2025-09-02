import argparse, sqlite3, csv, sys
from pathlib import Path

def export(db_path: str, out_csv: str):
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()
    cur.execute("""
        SELECT
          post_id, source, brand, created_utc, text,
          sentiment_score, sentiment_label, like_count,
          video_id, url
        FROM posts
        ORDER BY datetime(created_utc) ASC
    """)
    rows = cur.fetchall()
    Path(out_csv).parent.mkdir(parents=True, exist_ok=True)
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow([c for c in rows[0].keys()] if rows else
                   ["post_id","source","brand","created_utc","text","sentiment_score","sentiment_label","like_count","video_id","url"])
        for r in rows:
            w.writerow([r[k] for k in r.keys()])
    conn.close()
    print(f"✅ Exported {len(rows)} rows to {out_csv}")

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--db", required=True, help="Path to SQLite DB (e.g., storage/sentiment.db)")
    ap.add_argument("--out", required=True, help="Output CSV path (e.g., bi/powerbi/posts_latest.csv)")
    args = ap.parse_args()
    try:
        export(args.db, args.out)
    except Exception as e:
        print(f"Export failed: {e}", file=sys.stderr)
        sys.exit(1)


#!/usr/bin/env python3
import argparse, glob, json, os
from datetime import timezone
from dateutil import parser as dtparser
import pandas as pd
from vaderSentiment.vaderSentiment import SentimentIntensityAnalyzer

def load_ndjson_dir(in_dir: str) -> pd.DataFrame:
    rows = []
    for path in glob.glob(os.path.join(in_dir, "*.ndjson")):
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line: continue
                try: rows.append(json.loads(line))
                except: pass
    return pd.DataFrame(rows or [])

def clean_df(df: pd.DataFrame) -> pd.DataFrame:
    if df.empty: return df
    need = ["post_id","text","author","video_id","brand","keywords","created_utc","like_count","url","fetched_at"]
    for c in need:
        if c not in df.columns: df[c] = None
    df["text"] = df["text"].astype(str).str.replace(r"\s+"," ",regex=True).str.strip()
    df = df[df["text"].str.len() > 0].copy()
    def tparse(x):
        try: return dtparser.parse(str(x)).astimezone(timezone.utc).replace(tzinfo=None)
        except: return pd.NaT
    df["created_dt"] = df["created_utc"].apply(tparse)
    df["fetched_dt"] = df["fetched_at"].apply(tparse)
    if df["post_id"].notna().any():
        df = df.sort_values(["created_dt","fetched_dt"], ascending=True).drop_duplicates(subset=["post_id"], keep="first")
    else:
        df = df.drop_duplicates(subset=["video_id","author","text"], keep="first")
    def to_int(x):
        try: return int(x)
        except: return 0
    df["like_count"] = df["like_count"].apply(to_int)
    def kw_str(v):
        if isinstance(v, list): return ",".join(map(str, v))
        return "" if v is None else str(v)
    df["keywords_str"] = df["keywords"].apply(kw_str)
    return df

def add_sentiment(df: pd.DataFrame) -> pd.DataFrame:
    if df.empty: return df
    sid = SentimentIntensityAnalyzer()
    scores = df["text"].apply(lambda t: sid.polarity_scores(str(t)) if pd.notna(t) else {"compound":0.0})
    df["sent_compound"] = scores.apply(lambda s: s.get("compound",0.0))
    def label(c):
        if c >= 0.05: return "positive"
        if c <= -0.05: return "negative"
        return "neutral"
    df["sent_label"] = df["sent_compound"].apply(label)
    return df

def main():
    ap = argparse.ArgumentParser(description="NDJSON → clean CSV with sentiment")
    ap.add_argument("--in",  dest="in_dir",  default="../data/raw",     help="input dir with .ndjson")
    ap.add_argument("--out", dest="out_dir", default="../data/curated", help="output dir")
    ap.add_argument("--brand", dest="brand", default=None, help="optional brand filter")
    args = ap.parse_args()
    os.makedirs(args.out_dir, exist_ok=True)

    df = load_ndjson_dir(args.in_dir)
    if df.empty:
        print("No data found in", args.in_dir)
        return
    if args.brand and "brand" in df.columns:
        df = df[df["brand"].astype(str).str.lower() == args.brand.lower()]

    df = clean_df(df)
    df = add_sentiment(df)

    cols = ["post_id","brand","video_id","author","text","sent_label","sent_compound","like_count",
            "created_utc","created_dt","fetched_at","fetched_dt","keywords_str","url"]
    cols = [c for c in cols if c in df.columns] + [c for c in df.columns if c not in cols]
    df = df[cols]

    out_csv = os.path.join(args.out_dir, "comments_clean.csv")
    out_parquet = os.path.join(args.out_dir, "comments_clean.parquet")
    df.to_csv(out_csv, index=False)
    try: df.to_parquet(out_parquet, index=False)
    except: pass

    agg = df.groupby("sent_label").size().rename("count").reset_index()
    agg.to_csv(os.path.join(args.out_dir,"sentiment_counts.csv"), index=False)

    total=len(df)
    pos=(df["sent_label"]=="positive").sum() if "sent_label" in df.columns else 0
    neg=(df["sent_label"]=="negative").sum() if "sent_label" in df.columns else 0
    neu=(df["sent_label"]=="neutral").sum()  if "sent_label" in df.columns else 0
    print(f"Saved {total} rows. Pos={pos} Neg={neg} Neu={neu}")
    print("CSV:", out_csv)
    print("Parquet:", out_parquet, "(if created)")
    print("Counts:", os.path.join(args.out_dir,"sentiment_counts.csv"))

if __name__ == "__main__":
    main()



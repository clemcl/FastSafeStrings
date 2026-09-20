import random
import string
import sys

RECORDS = 5_000_000
FIELDS = 8

random.seed(42)

words = [
    "alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "golf", "hotel",
    "india", "juliet", "kilo", "lima", "mike", "november", "oscar", "papa",
    "quebec", "romeo", "sierra", "tango", "uniform", "victor", "whiskey",
    "xray", "yankee", "zulu", "status_active", "status_pending", "status_error",
    "status_complete", "processing", "completed", "initialized", "terminated",
    "PROC:", "DATA:", "INFO:", "WARN:", "ERROR:", "DEBUG:", "TRACE:", "AUDIT:",
]

statuses = ["ACTIVE", "PENDING", "ERROR", "COMPLETE", "PROCESSING"]

print(f"Generating {RECORDS} records to stdout...", file=sys.stderr)

for i in range(RECORDS):
    fields = []
    fields.append(str(i + 1))
    fields.append(random.choice(statuses))
    fields.append(''.join(random.choices(string.ascii_lowercase, k=random.randint(8, 20))))
    fields.append(str(random.randint(100, 99999)))
    fields.append(random.choice(words))
    fields.append(f"{random.uniform(0, 1000):.2f}")
    fields.append(''.join(random.choices(string.ascii_uppercase + string.digits, k=16)))
    fields.append(random.choice(words) + "_" + random.choice(words))
    print("|".join(fields))

print(f"Done.", file=sys.stderr)

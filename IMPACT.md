# Impact and Efficiency Considerations

FastSafeStrings reduces CPU work by eliminating repeated string scanning
(e.g. `strlen`, `strcat`, delimiter searches).

## Where this matters

The impact is most significant in workloads that process large volumes of text:

* log ingestion and analysis
* ETL / data pipelines
* financial batch processing
* mainframe-style record systems

In these environments, repeated scanning can dominate CPU time.

---

## Performance → Energy relationship

Reducing CPU work generally reduces energy consumption:

* fewer CPU cycles
* less memory bandwidth usage
* reduced heat generation

In data centres, lower compute load can also reduce cooling requirements.

---

## Order-of-magnitude estimate

At a global level, data centres consume hundreds of terawatt-hours (TWh) annually.

FastSafeStrings only affects a subset of workloads (those dominated by string processing).

A conservative estimate:

* string-heavy workloads: ~5–15% of compute
* potential efficiency gain in those workloads: ~10–30%

This suggests a possible impact on the order of:

👉 **~0.3% to 1% of total data centre energy usage**

This corresponds to roughly:

👉 **~1–5 TWh per year (globally)**

---

## Important caveats

* Not all workloads benefit (e.g. numerical or GPU-heavy systems)
* Impact depends heavily on application design
* Requires adoption in real systems
* Efficiency gains do not always translate directly into reduced total energy usage

---

## Summary

FastSafeStrings does not aim to reduce energy consumption directly.

However, by eliminating unnecessary repeated work in string handling,
it can improve efficiency in text-heavy systems — which at scale may
translate into measurable energy savings.

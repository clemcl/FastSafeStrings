## **Real-World Performance**

Processing 5,000,000 records: ~7� faster than standard C by eliminating repeated string scans.
 
FastSafeStrings is designed to eliminate repeated string scans — a common hidden cost in C programs.

To demonstrate this, we compared two approaches processing **5,000,000 records (\~35 bytes each)**.

---

### **Standard C approach**

while (fgets(buffer, MAX\_REC, fp)) {  
   
  size\_t len \= strlen(buffer);      // scan \#1  
   
  if (len \> 0 && buffer\[len-1\] \== '\\n')  
      buffer\[len-1\] \= '\\0';  
   
  strcpy(work\_area, "PROC:");      // scan \#2  
  strcat(work\_area, buffer);       // scan \#3  
}

This requires multiple passes over each record:

·        `fgets` scans for newline

·        `strlen` scans again

·        `strcat` scans the destination

---

### **FastSafeStrings \+ variable-blocked records**

while (VB\_Get(in, rec\_buf, 512, \&bytes\_read) \> 0\) {  
   
  dv\_rec\_buf.cur\_len \= bytes\_read;  
   
  SET(work\_area, "PROC:");  
  CAT(work\_area, rec\_buf);  
}

Here:

·        record length is known (no scanning)

·        string length is known (no `strlen`)

·        append is direct (`memcpy`)

---

### **Results**

Standard C (fgets/strcat)     : 1.506 seconds  
FastSafeStrings \+ VB records   : 0.205 seconds

👉 **\~7× faster**

---

## **Why this is faster**

Traditional C string processing repeatedly scans data:

read → scan → scan → scan

FastSafeStrings avoids scanning entirely:

read → process (no scans)

This reduces:

·        CPU work (fewer passes over memory)

·        branch overhead (no per-character checks)

·        cache pressure

---

## **Key Insight**

FastSafeStrings does not try to outperform `memcpy` or `strcmp`.

Instead, it improves performance by:

**eliminating unnecessary work**

Specifically:

·        No repeated `strlen` calls

·        No destination scanning for `strcat`

·        No delimiter scanning when record length is known

---

## **Safety Advantage**

In addition to performance, FastSafeStrings guarantees:

·        no buffer overruns

·        automatic truncation

·        predictable behaviour

The equivalent C version can overflow buffers if not carefully managed.

---

## **Summary**

This example shows the real benefit of FastSafeStrings:

**Length-aware data \+ length-aware strings \= fewer passes, safer code, and significant speedups**

FastSafeStrings improves performance not by replacing libc, but by avoiding the work libc must do.

 


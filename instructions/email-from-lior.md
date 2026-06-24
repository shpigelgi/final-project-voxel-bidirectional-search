# Project Brief — Email from Lior Siag

**From:** Lior Siag `<siagl@post.bgu.ac.il>`
**Date:** 16 Jun 2026
**Attachments:** 7 (the papers listed below — save them in `resources/papers/`)

---

## English translation

### Papers

1. A nice paper on an algorithm that *always meets in the middle*: **Holte**
2. A big-picture paper on the *theory of the lower bound (LB)* in bidirectional search: **Siag**
3. A paper that defines the *mathematics of which nodes must be expanded*: **Eckerle**
4. A paper that *extends* the above understanding: **Shaham**
5. A paper that *assumes the heuristic is consistent*: **Shaham 2018**
6. A paper presenting the *state of the art (SOTA)* in bidirectional search: **Alcazar**

> Reading order: Try paper **2 (Siag)** first. If it is clear, continue with it and then move to **3 (Eckerle)**. Otherwise, read the papers in their numbered order.

### Project

The goal of the project is to **run bidirectional search algorithms on Voxel maps**.
- If movement is already implemented, do it **both with and without diagonal movement**; if not, do it **without diagonal movement**.
- The core goal is to **connect the Warthog benchmark to the bidirectional search code in HOG2**.

**Links:**
- Warthog benchmark (voxel maps): https://bitbucket.org/shortestpathlab/benchmarks/src/master/voxel-maps/
- HOG2 git: https://github.com/MovingAILab/hog2
- How to use the HOG2 git (usage example): https://github.com/SPL-BGU/BiHS-Direction-Choosing/tree/master/src/paper
- Domain paper: *Voxel Benchmarks for 3D Pathfinding — Sandstone, Descent, and Industrial Plants*
- LLM-search work environment git: https://github.com/sumedhpendurkar/Search-LLM-inference

**Algorithms to run** (from both sides): **MM, BAE\*, A\*, BiA\*, GBFS**, and unidirectional algorithms.

Best regards,
Lior Siag

---

## Original (Hebrew)

> מאמרים:
> 1. מאמר נחמד של אלגוריתם שתמיד נפגש באמצע: Holte
> 3. מאמר שמגדיר את המתמטיקה של איזה קודקודים חייב לפתח: Eckerle
> 4. מאמר שמרחיב את ההבנה הנ"ל: Shaham
> 5. מאמר שמניח שההיוריסטיקה היא קונסיסטנטית: Shaham 2018
> 6. מאמר שמציג את ה-SOTA מבחינת חיפוש דו-כיווני: Alcazar
> 2. מאמר תמונה כללית על תיאוריה של ה-LB מבחינת חיפוש דו-כיווני: Siag
> נסה לקרוא את מאמר 2, אם הוא ברור תמשיך בו ואז ל-3. אם לא, תמשיך לפי סדר השורות.
>
> פרויקט:
> מטרת הפרויקט היא להריץ אלגוריתמי חיפוש דו-כיווני במפות Voxel. אם הדבר כבר ממומש אז גם עם תזוזה
> באלכסון וגם בלי, אם לא לא, ללא תזוזה באלכסון. המטרה היא לחבר בין benchmark של Warthog לקוד של
> חיפוש דו-כיווני ב-HOG2.
> לינק ל-benchmark ב-Warthog: https://bitbucket.org/shortestpathlab/benchmarks/src/master/voxel-maps/
> לינק ל-Hog2 גיט: https://github.com/MovingAILab/hog2
> קישור לצורת שימוש ב-Hog2 גיט: https://github.com/SPL-BGU/BiHS-Direction-Choosing/tree/master/src/paper
> מאמר בנושא של הדומיין: Voxel Benchmarks for 3D Pathfinding — Sandstone, Descent, and Industrial Plants
> האלגוריתמים הם: MM, BAE*, A*, BiA*, GBFS ואלגוריתמים חד-כיוונים, להריץ משני הצדדים.
> סביבת עבודה של חיפוש ב-LLM גיט: https://github.com/sumedhpendurkar/Search-LLM-inference
>
> בברכה,
> ליאור סייג.

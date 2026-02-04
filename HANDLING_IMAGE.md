# Handling Images (SquareLine Studio + LVGL 9)

This document captures what we learned about image widgets when integrating SquareLine Studio exports with LVGL 9 on this project. **If you add or edit images, read this to avoid the “80% white screen” issue and keep export and position correct.**

---

## TL;DR – Rule for images

- **Do not use Transform (style transform scale) on image widgets.**  
- **Use the image’s own scale:** In SquareLine Studio, use the **Image** section’s scale/zoom (the panel under the image widget), not the generic **Transform** scale.  
- That way the export uses `lv_image_set_scale()` and everything works with the theme and correct position.

---

## The problem we hit

### Setup

- **LVGL:** 9.1.0  
- **Display:** 320×480, default theme on  
- **Image:** One `lv_image` (e.g. profile photo), 256×247 px, `LV_COLOR_FORMAT_NATIVE_WITH_ALPHA`  
- **Scaling in export:** Image scale was exported as a **style** property:
  - `lv_obj_set_style_transform_scale(ui_Image1, 200, LV_PART_MAIN | LV_STATE_DEFAULT)`

### Symptom

- About **80% of the screen** appeared **white**.  
- Green screen, time label, and panel were correct; the white covered most of the display.  
- Removing the image **or** turning off the theme removed the white → issue was specific to **image + theme + style transform scale**.

### Root cause (what we inferred)

- The image was scaled with **`lv_obj_set_style_transform_scale()`** (generic object style), not with the image widget’s own API.  
- With the theme on, the theme paints a background (and other styles) on that image object. The **area** that gets styled was very large (e.g. unscaled or wrongly computed), so we saw a huge white rectangle even though the visible image was small.  
- So: **theme + image + style-based transform scale** → large white region.

---

## The fix (what works)

### In SquareLine Studio

1. **Do not use Transform for image scale**  
   Avoid setting scale/zoom via the generic **Transform** (or “Style → Transform scale”) on the image widget.

2. **Use the Image panel’s scale**  
   Select the image widget, then in the **Image** section (the panel under the image, not the generic Transform/Style), use the **scale/zoom** control there. That is the “normal” way to scale the image.  
   The exporter then generates **`lv_image_set_scale()`** instead of **`lv_obj_set_style_transform_scale()`**.

3. **Position**  
   Set position (x, y) and alignment as usual. With the image scaled via the Image panel, positions from the export will match on device.

### In code (after export)

- If the export already uses **`lv_image_set_scale(ui_Image1, 200)`** (or similar), **do nothing** – keep it.  
- If you still see old exports that use **`lv_obj_set_style_transform_scale()`** for the image:
  - **Replace** that line with **`lv_image_set_scale(ui_Image1, <value>)`** (use the same numeric scale value, often 200 or 180).  
  - Remove any style `transform_scale` for that image object so the theme doesn’t paint the big white rect.

---

## LVGL APIs (for reference)

- **Image widget (correct for scale):**
  - `lv_image_set_scale(obj, zoom)` – one zoom value  
  - `lv_image_set_scale_x()` / `lv_image_set_scale_y()` – per-axis  
  - `lv_image_set_rotation()`, `lv_image_set_pivot()`  
- **Generic object (avoids for image scale):**
  - `lv_obj_set_style_transform_scale()` – can cause the 80% white when theme is on.  
- There is **no** `lv_image_set_style_transform`; style transform is generic object API and is what caused the issue.

---

## What we tried that did *not* fix it

- Setting image **background alpha to 0** in SquareLine (theme still painted the large area).  
- Setting **bg_opa / border_opa to transparent** in code on the image while keeping **transform_scale** (white still appeared).  
- So the only reliable fix was **not using style transform scale on the image** and using **`lv_image_set_scale()`** (from SquareLine’s Image panel scale) instead.

---

## Summary table

| Approach | Result |
|----------|--------|
| Image scale via **Transform** (style) in SquareLine | 80% white screen with theme on |
| Image scale via **Image panel** scale in SquareLine | Export uses `lv_image_set_scale()` → no white, position correct |
| Replacing `transform_scale` with `lv_image_set_scale()` in code | Fixes white if export still used transform |

---

## Reminder for future UI work

**When adding or editing images in SquareLine Studio:**

- **Do not** use the generic **Transform** (or “Style → Transform scale”) to scale the image.  
- **Do** use the **Image** section (panel under the image widget) to set the image’s scale/zoom the normal way.  
- That keeps the export using `lv_image_set_scale()` and avoids the 80% white screen while keeping positions and layout as designed.

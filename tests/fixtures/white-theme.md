# White theme (personal)

White is this fork's eleventh built-in theme: plain black on pure white,
written as `resources/themes/white.css` and listed last in the chooser so a
saved theme index keeps pointing at the theme it did. Open this page with
White chosen and look at each part below.

## Text and emphasis

Body text in black, with **bold**, *italic*, ***both***, `inline code`,
~~struck~~ and ==marked== words, and a [link in plain blue](https://example.com).

### A third-level heading

#### A fourth-level heading

## Lists

- A bullet with **bold** text
  - A nested bullet with `code`
    - Three levels deep
- [ ] A task left open
- [x] A task done

1. First step
2. Second step, with a [link](https://example.com)

## Quotes and callouts

> A plain quote: its bar is light gray, its text a softer black.
>
> > A quote inside a quote.

> [!NOTE]
> A note keeps its own blue on the white page.

> [!WARNING]
> A warning keeps its amber.

## Table

| Theme | Page | Text |
|:------|:----:|-----:|
| Paper | cream | brown |
| White | `#FFFFFF` | `#000000` |

## Code

The block's light gray ground must show against the white page.

```cpp
// A comment, in green
int count(const std::string& text) {
    int n = 0;                 // a number, in teal
    for (char c : text)
        if (c == ' ') return n; // control flow, in purple
    return 42;
}
const char* s = "a string, in dark red";
```

---

A rule above, in light gray, and a footnote here.[^one]

[^one]: The footnote's text, in black.

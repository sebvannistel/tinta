# Raw HTML quotes around bare text

GitHub draws a raw HTML `<blockquote>` around bare text as a quote holding
that text, the same as one whose text sits in a `<p>`. Every quote below
shows its text beside a bar, and [the anchored quote](#quoted-anchor) is
one click away.

## One level

<blockquote><p>One level HTML quote paragraph</p></blockquote>

<blockquote>Bare text in an HTML quote</blockquote>

<blockquote>
Bare text on a line of its own, long enough to wrap at narrow widths, so the bar has to reach the last wrapped line and stop there.
</blockquote>

Paragraph between the one-level quotes and the inline tags.

## Inline tags and line breaks

<blockquote>Bare text with <b>bold</b>, <i>italic</i>, <code>inline code</code>, a <a href="https://example.com/quote">quoted link</a> and H<sub>2</sub>O,<br>
a second line after a break<br/>
and a third line.</blockquote>

<blockquote>Bare text before a paragraph
<p>A paragraph in the middle of the quote.</p>
Bare text after the paragraph.</blockquote>

<blockquote><a id="quoted-anchor"></a>
<p>A paragraph after an anchor that takes no room.</p></blockquote>

## Nested quotes

<blockquote>Outer bare text
<blockquote>Inner bare text
<blockquote><p>Innermost paragraph</p></blockquote>
Inner text after the innermost quote</blockquote>
Outer text after the inner quote</blockquote>

<blockquote><blockquote><blockquote>Three levels of bare text</blockquote></blockquote></blockquote>

## Beside Markdown quotes

> A Markdown quote before the HTML ones, with **bold** and `code`.
>
> > A nested Markdown quote.

<blockquote>Bare text between Markdown quotes</blockquote>

> A Markdown quote holding an HTML one:
>
> <blockquote>Bare text of an HTML quote inside a Markdown quote</blockquote>

> [!NOTE]
> A callout after the HTML quotes, with **bold** text.

## Lists, tables and code

- A list item with **bold** text
- A list item holding a quote:

  <blockquote>Bare text in a quote inside a list item</blockquote>

- A last list item with `code`

| Quote form | Shows its text |
| --- | --- |
| `<blockquote><p>Text</p></blockquote>` | Yes |
| `<blockquote>Text</blockquote>` | Yes |

```html
<blockquote>Code stays code, not a quote</blockquote>
```

1. An ordered list after the code
2. Its second item

All surrounding content stays in place.

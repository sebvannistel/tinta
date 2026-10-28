# Nested ~~struck x^2^~~ and ==marked H~2~O== spans

Obsidian/Typora spans nest inside each other. Every script below should be small
and raised or lowered, and keep the strikethrough line or highlight around it.
No `^`, `~` or `==` delimiter should remain visible outside code.

## Inside strikethrough and highlight

~~struck x^2^~~, ~~struck H~2~O~~, ==marked x^2^== and ==marked H~2~O==.

==A highlight with ~~struck words~~ inside== and ~~a strike with ==marked words== inside~~.

Shared closing runs: ~~a ~b~~~ ends the subscript and the strikethrough together,
while ~~a~~~ leaves one literal tilde and ~~x~~~y~ is strikethrough then subscript y.

Scripts inside scripts: x^a~i~^ and H~2^+^~, next to plain x^2^ and H~2~O.

**Bold ~~struck x^2^~~**, *italic ==marked H~2~O==*, ***both ~~e^iπ^~~***, and
[a link with ==marked x^2^==](#inside-strikethrough-and-highlight).

## Math and code stay opaque

==Highlighted $x^2 + y_1$ beside y^2^== and ~~struck $a==b$ next to ==marked H~2~O==~~.

Inline code keeps every delimiter: `~~struck x^2^~~ ==marked H~2~O==`.

```text
~~struck x^2^~~ and ==marked H~2~O== stay literal in a fence.
```

| Nesting | Superscript | Subscript | With math |
| --- | --- | --- | --- |
| Strikethrough | ~~x^2^~~ | ~~H~2~O~~ | ~~old $x^2$ and y^3^~~ |
| Highlight | ==x^2^== | ==H~2~O== | ==new $H_2O$ and CO~2~== |
| Both | ==~~x^2^~~== | ~~==H~2~O==~~ | `==~~x^2^~~==` |

> A quote with ~~struck x^2^~~, ==marked H~2~O==, **bold**, `code` and $a+b=c$.
>
> This second paragraph must keep its normal spacing.

- List item with ~~struck x^2^~~ and ==marked H~2~O==.
- Nested list parent:
  - Child with ==a ~~b^c^~~ d== inside.
- A longer item that wraps in a narrow window: ~~the reaction released CO~2~ and heat at 10^5^ Pa~~ while ==the marked span keeps x^2^, H~2~O and $E=mc^2$ on its background== through the wrapped lines.

1. Ordered item with ~~a ~b~~~ shared run.
2. Ordered item with ==x^a~i~^== double nesting.

### Smaller heading ==x^2^== and ~~H~2~O~~

$$
\frac{x^2 + y^2}{H_2O}
$$

All surrounding content must remain visible, including this final paragraph.

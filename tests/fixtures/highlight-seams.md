# Highlight seams ==marked x^2^== in a heading

Every highlight below should be one even band from its first to its last
character: no darker strip where a script, a struck word, a link or an equation
starts or ends, and no pale gap at the spaces between them. Check Paper and
Midnight, which paints the most translucent band.

## Pieces of one highlight

==marked x^2^==, ==marked H~2~O==, ==highlighted $\sqrt{x}$ result== and
==tight ($x$) parentheses==.

==A highlight with ~~struck words~~ inside==, ==a [[Wiki Page]] link== and
==e^iπ^ + 1 = 0 beside $\dfrac{1}{1+x^2}$ and CO~2~==.

Neighbouring highlights may touch at small sizes but never paint a darker strip
where they meet: ==first== ==second== and ==third==, ==fourth==.

**Bold ==marked x^2^==**, *italic ==marked H~2~O==* and
[a link with ==marked x^2^==](#pieces-of-one-highlight).

## Wrapped highlight

A highlight cannot cross a source line break, so this one is a single line:
==this long highlight keeps x^2^, H~2~O, ~~struck text~~ and $E=mc^2$ on its background while it wraps over several lines in a narrow window, and each wrapped line starts and ends with the same small padding as a short highlight, then runs on through 10^5^ Pa and $\sqrt[3]{8}=2$ to the end.==

## Surrounding blocks

| Highlight | With a script | With math |
| --- | --- | --- |
| ==plain== | ==x^2^ and H~2~O== | ==new $H_2O$ and CO~2~== |
| `==literal==` | **==bold y^3^==** | ==$\frac{a}{b}$ fraction== |

> A quote with ==marked x^2^ and $a+b=c$==, **bold**, `code` and a
> [link](#surrounding-blocks).
>
> The second quoted paragraph keeps its spacing.

- List item with ==marked H~2~O== and ==$\alpha$ first==.
- Nested parent:
  - Child with ==a ~~b^c^~~ d== inside.

1. Ordered item with ==x^a~i~^== double nesting.
2. Ordered item with plain ==highlight== text.

```text
==marked x^2^== stays literal in a fence.
```

$$
\frac{x^2 + y^2}{H_2O}
$$

All surrounding content must remain visible, including this final paragraph.

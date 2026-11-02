# Encoded local image paths (#293)

These **images** combine *emphasis*, `inline code`, and a [local link](index.md).
Every image below must render, including in HTML and Word exports.

![Plain path](plain.png)

![Spaces in directory](folder%20with%20space/image.png)

![UTF-8 filename](caf%C3%A9.png)

![Hash in filename](hash%23name.png)

![Literal percent escape, decoded once](literal%2520.png)

| Image | Expected behavior |
| --- | --- |
| ![Literal plus](plus+name.png) | Plus remains a plus, rather than a space. |

> ![Malformed escape](malformed%zz.png)
>
> Invalid escapes remain literal.

- ![Encoded NUL remains literal](nul%00.png)
- ![Encoded extension](plain.%70%6e%67)

```markdown
![Spaces in directory](folder%20with%20space/image.png)
```

## Final heading

Surrounding content survives.

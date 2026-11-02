# Images and links on other computers

A document shows **images** from its own folder, from paths on this
computer and from the server it was opened from. An image or file link
that names *another* computer's share stays unopened: opening it would
sign in to that server with your Windows credentials.

![Local image](plain.png)

![Forward slashes](//tinta-blocked.invalid/share/shot.png)

![Backslashes](\\\\tinta-blocked.invalid\share\shot.png)

![Mixed slashes](/\tinta-blocked.invalid/share/shot.png)

![Encoded](%5C%5Ctinta-blocked.invalid%5Cshare%5Cshot.png)

![Long form](//?/UNC/tinta-blocked.invalid/share/shot.png)

| Kind | Image |
| --- | --- |
| Table cell | ![In a table](//tinta-blocked.invalid/share/table.png) |

- In a list: ![In a list](//tinta-blocked.invalid/share/list.png)

> ![In a quote](//tinta-blocked.invalid/share/quote.png)
>
> The note in each placeholder says why the image is missing.

File links to another computer badge as missing without a lookup:
[the share's notes](/\tinta-blocked.invalid/share/notes.md), the plain
reference /\tinta-blocked.invalid/share/plain.md and the wiki link
[[//tinta-blocked.invalid/share/wiki]]. The local
[linked note](linked.md) stays live.

```markdown
![Forward slashes](//tinta-blocked.invalid/share/shot.png)
```

## Closing heading

Surrounding content survives.

# Byte order mark

This file starts with a UTF-8 byte order mark: the bytes `EF BB BF` that
Windows PowerShell 5.1 (`Set-Content -Encoding utf8`), Visual Studio and
some editors write ahead of the text. The mark is not text, so the line
above is a **heading**, not the literal `# Byte order mark`, and nothing
shows above it. The [tasks](#tasks) at the end flip in the file when
clicked.

## Lists

- A bullet with *emphasis*
  - A nested bullet with `inline code`
- A bullet with a [link](https://example.com/bom)

1. Save the file as UTF-8 with BOM
2. Open it in Tinta

## Table

| Encoding | First bytes | First line |
| --- | --- | --- |
| UTF-8 | `23 20` | Heading |
| UTF-8 with BOM | `EF BB BF 23 20` | Heading, mark skipped |

## Quote

> A quote keeps its bar and its **bold**.
>
> > A nested quote.

## Code

```powershell
Set-Content -Path doc.md -Value '# Title' -Encoding utf8
```

## Tasks

- [ ] Click this box: its mark flips in the file and the byte order mark stays
- [x] A checked task
- [ ] A last task with *emphasis*

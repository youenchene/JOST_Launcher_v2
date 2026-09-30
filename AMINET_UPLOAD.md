# Uploading a release to Aminet

`jl` is published at https://aminet.net/package/util/misc/jl. Each release
is an update of that package. Rules:
https://wiki.aminet.net/Uploading_instructions and
https://wiki.aminet.net/The_Readme_file.

Do this after the GitHub release (see "Release" in `AGENTS.md`) and after
testing on real hardware. Aminet asks for at most one update per week.

## What gets uploaded

Two files, with the same base name and case:

- `jl.lha`: the release archive (`jl`, `jl-menu`, `jl-config.cfg`, `jl.readme`)
- `jl.readme`: the same file as `dist/jl.readme`

Upload the files published by the GitHub release, not a local build, so
Aminet has what was tested. Uploading `jl.lha` again under the same name
replaces the old version: no `Replaces:` field is needed.

## Checks before uploading

- `jl.readme` has `Short:`, `Uploader:`, `Type: util/misc`, `Version:` (the
  new one), `Architecture: m68k-amigaos >= 1.3` and `Distribution: Aminet`.
- `Short:` is 40 characters or fewer.
- No line longer than 78 characters, LF line endings (no CR):
  `awk 'length($0)>78' dist/jl.readme` and `grep -c $'\r' dist/jl.readme`
  must print nothing / 0.
- The archive has no copyrighted or Workbench material. List it with
  `lha l jl.lha` (there is no local `lha`: use the toolchain image,
  `docker run --rm --platform linux/amd64 -v "$PWD:/w" amigadev/crosstools:m68k-amigaos lha l /w/jl.lha`).

## Upload

```sh
mkdir -p /tmp/aminet && cd /tmp/aminet
gh release download vX.Y -p jl.lha -p jl.readme --clobber

# anonymous FTP; the password is the uploader's email address
curl --ftp-pasv -T jl.lha    'ftp://anonymous:EMAIL%40DOMAIN@main.aminet.net/new/jl.lha'
curl --ftp-pasv -T jl.readme 'ftp://anonymous:EMAIL%40DOMAIN@main.aminet.net/new/jl.readme'

# check both files are there
curl --ftp-pasv -l 'ftp://anonymous:EMAIL%40DOMAIN@main.aminet.net/new/' | grep '^jl\.'
```

Write the `@` of the email as `%40` in the URL. The email is the one in the
`Uploader:` field of the readme.

## After the upload

- Aminet staff review it, then move it to `util/misc`: it takes about a day.
  Check https://aminet.net/package/util/misc/jl shows the new version.
- If there is a problem, they write to the `Uploader:` address.
- To fix a mistake, upload again under the same name.

## History

- 2.1: first upload of the C rewrite line (2.0 was GitHub only).

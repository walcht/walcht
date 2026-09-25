## Setup

```shell
gem install jekyll bundler
```

## Run Locally

```shell
bundle exec jekyll serve
```

## Deploy

```shell
JEKYLL_ENV=production bundle exec jekyll build
```

## Notes

Ruby on Rails and this whole gem ecosystem is a hot pile of garbage. Backwards
compatiblity is not considered for newer versions of Ruby (>3.2.0) and
everything fails if you install a newer version.

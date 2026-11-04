#!/bin/bash
# SONIC GRAIN - crea e pubblica il tag di release (vX.Y.Z) per la versione in CMakeLists.txt.
# Doppio clic dal Finder, o da Terminale: scripts/push-tag.command
# La CI, ricevuto il tag, compila e pubblica la Release su GitHub.
cd "$(dirname "$0")/.."
fail() { echo ""; echo "!! ERRORE: $1"; echo "(puoi chiudere questa finestra)"; exit 1; }

VERSION=$(sed -nE 's/^project\(SonicGrain VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt)
[ -n "$VERSION" ] || fail "versione non trovata in CMakeLists.txt"
TAG="v$VERSION"
echo "=== SONIC GRAIN release $TAG"

# 1) deve essere main, pulito e allineato a GitHub
BRANCH=$(git rev-parse --abbrev-ref HEAD)
[ "$BRANCH" = "main" ] || fail "sei sul branch '$BRANCH': passa a main (git checkout main)"
[ -z "$(git status --porcelain --untracked-files=no)" ] || fail "ci sono modifiche non salvate: fai commit prima del tag"
echo ">> Aggiornamento da GitHub..."
git fetch --quiet origin main --tags || fail "impossibile contattare GitHub"
[ "$(git rev-parse HEAD)" = "$(git rev-parse origin/main)" ] || fail "main locale e main su GitHub non coincidono: fai git pull / git push"

# 2) il CHANGELOG deve avere la voce di questa versione
grep -q "^## \[$VERSION\]" CHANGELOG.md || fail "manca la voce '## [$VERSION]' in CHANGELOG.md"

# 3) il tag non deve esistere gia'
if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null || git ls-remote --exit-code --tags origin "$TAG" >/dev/null 2>&1; then
  fail "il tag $TAG esiste gia': aggiorna la versione in CMakeLists.txt e nel CHANGELOG"
fi

# 4) conferma, poi tag annotato e push
echo ""
echo "Commit: $(git log --oneline -1)"
read -r -p "Creo e pubblico $TAG? [s/N] " ok
case "$ok" in s|S|si|SI|y|Y) ;; *) echo "Annullato."; exit 0 ;; esac
git tag -a "$TAG" -m "SONIC GRAIN $VERSION" || fail "creazione del tag non riuscita"
if ! git push origin "$TAG"; then
  git tag -d "$TAG" >/dev/null
  fail "push del tag non riuscito (il tag locale e' stato rimosso)"
fi
echo ""
echo "OK: $TAG pubblicato. La CI ora compila e pubblica la Release."
echo "(puoi chiudere questa finestra)"

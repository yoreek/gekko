import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

const script = fileURLToPath(new URL('./fix-doc-links.mjs', import.meta.url));

test('accepts verified English pages, assets, anchors, public files, external URLs, and service routes', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/assets/images/example.svg', '<svg/>');
  fixture.write('public/favicon.svg', '<svg/>');
  fixture.write(
    'src/content/docs/getting-started/source.mdx',
    `# Source

[Target](/gekko/guides/target/#target-heading)
![Asset](../../../assets/images/example.svg)
[Same page](#source)
[Public file](/favicon.svg)
[External](https://example.com/docs)
<LinkButton href="/gekko/install/">Install</LinkButton>
`,
  );
  fixture.write('src/content/docs/guides/target.md', '# Target heading\n');

  const result = fixture.run('--check', 'src/content/docs');

  assert.equal(result.status, 0, result.output);
  assert.match(result.output, /"changedFiles": 0/);
  assert.match(result.output, /"unresolved": 0/);
  assert.match(result.output, /"linksIgnored": 2/);
});

test('check fails for a resolvable image path that needs another parent level', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/assets/diagrams/analog-chain.svg', '<svg/>');
  fixture.write('src/content/docs/fr/index.md', '# Accueil\n');
  fixture.write(
    'src/content/docs/fr/reference/devices/analog-outputs.md',
    '![Chaîne](../../../../assets/diagrams/analog-chain.svg)\n',
  );

  const wrongPath = '../../../../assets/diagrams/analog-chain.svg';
  const correctPath = '../../../../../assets/diagrams/analog-chain.svg';
  const original = fixture.read('src/content/docs/fr/reference/devices/analog-outputs.md');

  const check = fixture.run('--check', 'src/content/docs/fr');
  assert.equal(check.status, 1, check.output);
  assert.match(check.output, /NEEDS_REWRITE:/);
  assert.match(check.output, /analog-outputs\.md:1/);
  assert.ok(check.output.includes(`${wrongPath} => ${correctPath}`), check.output);
  assert.match(check.output, /"changedFiles": 1/);
  assert.match(check.output, /"linksRewritten": 1/);
  assert.equal(fixture.read('src/content/docs/fr/reference/devices/analog-outputs.md'), original);

  const write = fixture.run('--write', 'src/content/docs/fr');
  assert.equal(write.status, 0, write.output);
  assert.equal(
    fixture.read('src/content/docs/fr/reference/devices/analog-outputs.md'),
    `![Chaîne](${correctPath})\n`,
  );

  const finalCheck = fixture.run('--check', 'src/content/docs/fr');
  assert.equal(finalCheck.status, 0, finalCheck.output);
  assert.match(finalCheck.output, /"changedFiles": 0/);
  assert.match(finalCheck.output, /"linksRewritten": 0/);
});

test('check fails for a resolvable image path with one excess parent level', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/assets/diagrams/analog-chain.svg', '<svg/>');
  fixture.write(
    'src/content/docs/reference/devices/analog-outputs.md',
    '![Chain](../../../../../assets/diagrams/analog-chain.svg)\n',
  );

  const check = fixture.run('--check', 'src/content/docs/reference/devices/analog-outputs.md');

  assert.equal(check.status, 1, check.output);
  assert.match(check.output, /NEEDS_REWRITE:/);
  assert.match(
    check.output,
    /\.\.\/\.\.\/\.\.\/\.\.\/\.\.\/assets\/diagrams\/analog-chain\.svg => \.\.\/\.\.\/\.\.\/\.\.\/assets\/diagrams\/analog-chain\.svg/,
  );
});

test('check fails when a localized document still links to the English page', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/content/docs/guides/target.md', '# Target\n');
  fixture.write('src/content/docs/fr/index.md', '# Accueil\n');
  fixture.write('src/content/docs/fr/guides/target.md', '# Cible\n');
  fixture.write(
    'src/content/docs/fr/guides/source.md',
    '[Cible](/gekko/guides/target/)\n',
  );

  const check = fixture.run('--check', 'src/content/docs/fr');

  assert.equal(check.status, 1, check.output);
  assert.match(check.output, /NEEDS_REWRITE:/);
  assert.match(check.output, /\/gekko\/guides\/target\/ => \/gekko\/fr\/guides\/target\//);
});

test('rewrites a page, translated anchor, image, reference definition, and static MDX href only after resolving targets', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/assets/images/example.svg', '<svg/>');
  fixture.write('src/content/docs/guides/target.md', '# Target heading\n');
  fixture.write('src/content/docs/ru/index.mdx', '# Главная\n');
  fixture.write('src/content/docs/ru/guides/target.md', '# Целевой раздел\n');
  fixture.write(
    'src/content/docs/ru/getting-started/source.mdx',
    `---
link: /gekko/guides/target/
file: ../../../assets/images/example.svg
---

# Источник

[Страница](/gekko/guides/target/#target-heading)
[Многострочная
ссылка](/gekko/guides/target/)
![Картинка](../../../assets/images/example.svg)
[справочная ссылка]: /gekko/guides/target/
<LinkButton href="/gekko/guides/target/">Страница</LinkButton>
`,
  );

  const original = fixture.read('src/content/docs/ru/getting-started/source.mdx');
  const dryRun = fixture.run('--check', 'src/content/docs/ru');
  assert.equal(dryRun.status, 1, dryRun.output);
  assert.match(dryRun.output, /"linksRewritten": 7/);
  assert.match(
    dryRun.output,
    /NEEDS_REWRITE: src\/content\/docs\/ru\/getting-started\/source\.mdx:\d+ -> \.\.\/\.\.\/\.\.\/assets\/images\/example\.svg => \.\.\/\.\.\/\.\.\/\.\.\/assets\/images\/example\.svg/,
  );
  assert.equal(fixture.read('src/content/docs/ru/getting-started/source.mdx'), original);

  const writeRun = fixture.run('--write', 'src/content/docs/ru');
  assert.equal(writeRun.status, 0, writeRun.output);
  const source = fixture.read('src/content/docs/ru/getting-started/source.mdx');
  assert.match(source, /\/gekko\/ru\/guides\/target\/#целевой-раздел/);
  assert.match(source, /\[Многострочная\nссылка\]\(\/gekko\/ru\/guides\/target\/\)/);
  assert.match(source, /!\[Картинка\]\(\.\.\/\.\.\/\.\.\/\.\.\/assets\/images\/example\.svg\)/);
  assert.match(source, /\[справочная ссылка\]: \/gekko\/ru\/guides\/target\//);
  assert.match(source, /href="\/gekko\/ru\/guides\/target\/"/);
  assert.match(source, /link: \/gekko\/ru\/guides\/target\//);
  assert.match(source, /file: \.\.\/\.\.\/\.\.\/\.\.\/assets\/images\/example\.svg/);

  const finalCheck = fixture.run('--check', 'src/content/docs/ru');
  assert.equal(finalCheck.status, 0, finalCheck.output);
  assert.match(finalCheck.output, /"changedFiles": 0/);
  assert.match(finalCheck.output, /"linksRewritten": 0/);
});

test('reports missing pages, assets, anchors, localized pages, and unsupported root routes', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/content/docs/guides/target.md', '# Existing heading\n');
  fixture.write('src/content/docs/ru/index.mdx', '# Главная\n');
  fixture.write(
    'src/content/docs/ru/getting-started/source.md',
    `# Источник

[Missing page](/gekko/guides/missing/)
![Missing asset](../../../../assets/images/missing.svg)
[Missing anchor](/gekko/guides/target/#missing-heading)
[Missing translation](/gekko/guides/target/)
[Unsupported root](/docs/file.md)
`,
  );

  const result = fixture.run('--check', 'src/content/docs/ru');

  assert.equal(result.status, 1, result.output);
  assert.match(result.output, /UNRESOLVED\[missing-page\]/);
  assert.match(result.output, /UNRESOLVED\[missing-asset\]/);
  assert.match(result.output, /UNRESOLVED\[locale-mismatch\]/);
  assert.match(result.output, /UNRESOLVED\[unsupported-root-route\]/);

  const original = fixture.read('src/content/docs/ru/getting-started/source.md');
  const writeResult = fixture.run('--write', 'src/content/docs/ru');
  assert.equal(writeResult.status, 1, writeResult.output);
  assert.equal(fixture.read('src/content/docs/ru/getting-started/source.md'), original);
});

test('reports a missing anchor when the localized target page exists', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/content/docs/guides/target.md', '# Existing heading\n');
  fixture.write('src/content/docs/ru/index.mdx', '# Главная\n');
  fixture.write('src/content/docs/ru/guides/target.md', '# Существующий раздел\n');
  fixture.write(
    'src/content/docs/ru/getting-started/source.md',
    '[Missing anchor](/gekko/guides/target/#missing-heading)\n',
  );

  const result = fixture.run('--check', 'src/content/docs/ru');

  assert.equal(result.status, 1, result.output);
  assert.match(result.output, /UNRESOLVED\[missing-anchor\]/);
});

test('refuses to map an anchor when translated heading structure differs', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/content/docs/guides/target.md', '# Target\n\n## Details\n');
  fixture.write('src/content/docs/ru/index.mdx', '# Главная\n');
  fixture.write('src/content/docs/ru/guides/target.md', '# Цель\n\n### Детали\n');
  fixture.write(
    'src/content/docs/ru/getting-started/source.md',
    '[Details](/gekko/guides/target/#details)\n',
  );

  const result = fixture.run('--check', 'src/content/docs/ru');

  assert.equal(result.status, 1, result.output);
  assert.match(result.output, /UNRESOLVED\[anchor-structure-mismatch\]/);
});

test('reports ambiguous page routes instead of choosing between md and mdx', (t) => {
  const fixture = createFixture(t);
  fixture.write('src/content/docs/guides/target.md', '# Markdown\n');
  fixture.write('src/content/docs/guides/target.mdx', '# MDX\n');
  fixture.write('src/content/docs/getting-started/source.md', '[Target](/gekko/guides/target/)\n');

  const result = fixture.run('--check', 'src/content/docs');

  assert.equal(result.status, 1, result.output);
  assert.match(result.output, /AMBIGUOUS:/);
  assert.match(result.output, /guides\/target\.md/);
  assert.match(result.output, /guides\/target\.mdx/);
});

function createFixture(t) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'gekko-doc-links-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));

  return {
    write(relative, content) {
      const absolute = path.join(root, relative);
      fs.mkdirSync(path.dirname(absolute), { recursive: true });
      fs.writeFileSync(absolute, content);
    },
    read(relative) {
      return fs.readFileSync(path.join(root, relative), 'utf8');
    },
    run(...args) {
      const result = spawnSync(process.execPath, [script, ...args], {
        cwd: root,
        encoding: 'utf8',
      });
      return {
        status: result.status,
        output: `${result.stdout ?? ''}${result.stderr ?? ''}${result.error?.stack ?? ''}`,
      };
    },
  };
}

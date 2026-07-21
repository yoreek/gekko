#!/usr/bin/env node

import fs from 'node:fs/promises';
import fsSync from 'node:fs';
import GithubSlugger from 'github-slugger';
import path from 'node:path';
import process from 'node:process';

const FILE_EXTENSIONS = new Set(['.md', '.mdx']);
const ASSET_EXTENSIONS = new Set([
  '.png',
  '.jpg',
  '.jpeg',
  '.gif',
  '.svg',
  '.webp',
  '.avif',
  '.bmp',
  '.ico',
]);
const ENGLISH_DOC_SECTIONS = new Set(['getting-started', 'guides', 'reference']);
const DOCS_BASE = '/gekko';

// These routes are generated outside src/content/docs and therefore cannot be
// resolved to a localized Markdown file. Keep this list explicit: a generic
// exception for all absolute paths would hide broken documentation links.
const SERVICE_ROUTES = new Set(['/gekko/install/']);

const args = parseArgs(process.argv.slice(2));
const repoRoot = process.cwd();
const docsRoot = path.resolve(repoRoot, 'src/content/docs');
const assetsRoot = path.resolve(repoRoot, 'src/assets');
const publicRoot = path.resolve(repoRoot, 'public');
const localeNames = discoverLocaleNames(docsRoot);
const defaultTargets = args.targets.length > 0 ? args.targets : ['src/content/docs'];

const summary = {
  files: 0,
  changedFiles: 0,
  linksChecked: 0,
  linksIgnored: 0,
  linksRewritten: 0,
  rewrites: [],
  unresolved: [],
  ambiguous: [],
};

for (const target of defaultTargets) {
  const absTarget = path.resolve(repoRoot, target);
  const stat = await fs.stat(absTarget).catch(() => null);
  if (!stat) {
    throw new Error(`Target not found: ${target}`);
  }

  const files = stat.isDirectory()
    ? await collectDocsFiles(absTarget)
    : FILE_EXTENSIONS.has(path.extname(absTarget).toLowerCase())
      ? [absTarget]
      : [];

  for (const file of files) {
    summary.files += 1;
    const original = await fs.readFile(file, 'utf8');
    const rewritten = rewriteFile(original, file, {
      docsRoot,
      assetsRoot,
      publicRoot,
      localeNames,
      summary,
    });

    if (rewritten.changed && args.write) {
      await fs.writeFile(file, rewritten.text, 'utf8');
    }

    if (rewritten.changed) {
      summary.changedFiles += 1;
    }
  }
}

if (args.write === false && summary.rewrites.length > 0) {
  for (const item of summary.rewrites) {
    console.error(
      `NEEDS_REWRITE: ${relativeForOutput(item.file)}:${item.line} -> ${item.target} => ${item.rewritten}`,
    );
  }
}

if (
  summary.unresolved.length > 0 ||
  summary.ambiguous.length > 0 ||
  (args.write === false && summary.rewrites.length > 0)
) {
  for (const item of summary.unresolved) {
    console.error(`UNRESOLVED[${item.reason}]: ${relativeForOutput(item.file)}:${item.line} -> ${item.target}`);
  }
  for (const item of summary.ambiguous) {
    console.error(`AMBIGUOUS: ${relativeForOutput(item.file)}:${item.line} -> ${item.target}`);
    for (const candidate of item.candidates) {
      console.error(`  ${relativeForOutput(candidate)}`);
    }
  }
  process.exitCode = 1;
}

console.log(
  JSON.stringify(
    {
      files: summary.files,
      changedFiles: summary.changedFiles,
      linksChecked: summary.linksChecked,
      linksIgnored: summary.linksIgnored,
      linksRewritten: summary.linksRewritten,
      unresolved: summary.unresolved.length,
      ambiguous: summary.ambiguous.length,
      mode: args.write ? 'write' : 'check',
    },
    null,
    2,
  ),
);

function parseArgs(argv) {
  const result = {
    write: false,
    targets: [],
  };

  for (const value of argv) {
    if (value === '--check') {
      result.write = false;
      continue;
    }
    if (value === '--write') {
      result.write = true;
      continue;
    }
    if (value === '--help' || value === '-h') {
      printHelpAndExit();
    }
    result.targets.push(value);
  }

  return result;
}

function printHelpAndExit() {
  console.log(`Usage:
  node scripts/fix-doc-links.mjs [--write|--check] [path ...]

Defaults:
  - checks only; pass --write to apply verified replacements
  - scans src/content/docs when no path is provided

Behavior:
  - checks Markdown links/images, reference definitions, frontmatter link/file
    values, and static MDX href/src attributes
  - checks that every internal page, asset, public file, and heading anchor exists
  - rewrites an asset path only after resolving the real file under src/assets
  - rewrites page links to the current document locale only after resolving the
    real localized Markdown/MDX file
  - ignores external URLs and the explicit service-route allowlist only
  - reports missing, wrong-locale, invalid-anchor, and ambiguous targets and
    exits with code 1`);
  process.exit(0);
}

async function collectDocsFiles(startDir) {
  const entries = await fs.readdir(startDir, { withFileTypes: true });
  const files = [];

  for (const entry of entries) {
    const absolute = path.join(startDir, entry.name);
    if (entry.isDirectory()) {
      files.push(...(await collectDocsFiles(absolute)));
      continue;
    }
    if (FILE_EXTENSIONS.has(path.extname(entry.name).toLowerCase())) {
      files.push(absolute);
    }
  }

  return files.sort();
}

function rewriteFile(text, file, context) {
  const lines = text.split('\n');
  let changed = false;
  let inFence = false;
  let fenceMarker = null;
  let inFrontmatter = false;
  let frontmatterEnded = false;

  const replaceTarget = (target, lineNumber) => {
    const result = fixTarget(target, file, context);
    if (result.ignored) {
      context.summary.linksIgnored += 1;
      return target;
    }

    context.summary.linksChecked += 1;
    if (!result.ok) {
      if (result.reason === 'ambiguous') {
        context.summary.ambiguous.push({
          file,
          line: lineNumber,
          target,
          candidates: result.candidates,
        });
      } else {
        context.summary.unresolved.push({
          file,
          line: lineNumber,
          target,
          reason: result.reason,
        });
      }
      return target;
    }

    if (result.rewritten !== target) {
      changed = true;
      context.summary.linksRewritten += 1;
      context.summary.rewrites.push({
        file,
        line: lineNumber,
        target,
        rewritten: result.rewritten,
      });
    }
    return result.rewritten;
  };

  const rewritten = lines.map((line, index) => {
    const lineNumber = index + 1;

    if (!frontmatterEnded && line.trim() === '---') {
      inFrontmatter = !inFrontmatter;
      if (!inFrontmatter) {
        frontmatterEnded = true;
      }
      return line;
    }

    if (inFrontmatter) {
      return line.replace(
        /^(\s*(?:-\s*)?(?:link|file):\s*)([^\n]+?)(\s*)$/,
        (match, prefix, rawTarget, suffix) => {
          const target = stripWrappingQuotes(rawTarget.trim());
          if (!target) {
            return match;
          }
          const replacement = replaceTarget(target, lineNumber);
          return `${prefix}${wrapIfQuoted(rawTarget, replacement)}${suffix}`;
        },
      );
    }

    const fenceMatch = line.match(/^(\s*)(`{3,}|~{3,})/);
    if (fenceMatch) {
      const marker = fenceMatch[2][0];
      if (!inFence) {
        inFence = true;
        fenceMarker = marker;
      } else if (marker === fenceMarker) {
        inFence = false;
        fenceMarker = null;
      }
      return line;
    }

    if (inFence) {
      return line;
    }

    let result = line;

    // Inline Markdown links and images. Matching from `](` also handles a link
    // label that starts on a previous line. The optional trailing part
    // preserves a Markdown title such as: [label](target "title").
    result = result.replace(
      /(\]\()([^\s)]+)([^)]*)(\))/g,
      (match, prefix, target, rest, suffix) =>
        `${prefix}${replaceTarget(stripAngleBrackets(target), lineNumber)}${rest}${suffix}`,
    );

    // Reference-style Markdown definitions: [name]: target "optional title"
    result = result.replace(
      /^(\s*\[[^\]]+\]:\s*)(<?[^\s>]+>?)(.*)$/,
      (match, prefix, rawTarget, rest) => {
        const target = stripAngleBrackets(rawTarget);
        return `${prefix}${replaceTarget(target, lineNumber)}${rest}`;
      },
    );

    // Static href/src values in MDX components. Dynamic expressions are left
    // untouched because they cannot be resolved safely without executing code.
    result = result.replace(
      /(\b(?:href|src)\s*=\s*)(["'])([^"']+)(["'])/g,
      (match, prefix, openingQuote, target, closingQuote) =>
        `${prefix}${openingQuote}${replaceTarget(target, lineNumber)}${closingQuote}`,
    );

    return result;
  });

  return {
    changed,
    text: rewritten.join('\n'),
  };
}

function fixTarget(target, file, context) {
  const parsed = splitTarget(target);

  if (isExternalTarget(parsed.pathPart)) {
    return { ignored: true };
  }

  if (!parsed.pathPart) {
    if (!parsed.fragment) {
      return { ignored: true };
    }
    const anchor = resolveAnchor({
      requestedFile: file,
      desiredFile: file,
      fragment: parsed.fragment,
    });
    if (!anchor.ok) {
      return anchor;
    }
    return {
      ok: true,
      rewritten: composeTarget('', parsed.query, anchor.fragment),
    };
  }

  const normalizedServiceRoute = normalizeServiceRoute(parsed.pathPart);
  if (SERVICE_ROUTES.has(normalizedServiceRoute)) {
    return { ignored: true };
  }

  if (isDocsRoute(parsed.pathPart)) {
    return fixAbsoluteDocsRoute(parsed, file, context);
  }

  if (parsed.pathPart.startsWith('/')) {
    const publicFile = path.resolve(context.publicRoot, `.${parsed.pathPart}`);
    if (isInside(context.publicRoot, publicFile) && isFile(publicFile)) {
      return { ok: true, rewritten: target };
    }
    return { ok: false, reason: 'unsupported-root-route' };
  }

  return fixRelativeTarget(parsed, file, context);
}

function fixAbsoluteDocsRoute(parsed, file, context) {
  const docContext = getDocContext(file, context.docsRoot);
  const requestedRoute = routeInsideDocsBase(parsed.pathPart);
  const routeInfo = splitRouteLocale(requestedRoute, context.localeNames);
  const contentRoute = routeInfo.contentRoute;
  const requestedRoot =
    routeInfo.locale === 'en'
      ? context.docsRoot
      : path.join(context.docsRoot, routeInfo.locale);
  const desiredRoot = docContext.localeRoot;
  const requestedMatches = findDocFileForRoute(requestedRoot, contentRoute);
  const desiredMatches = findDocFileForRoute(desiredRoot, contentRoute);

  if (requestedMatches.length > 1 || desiredMatches.length > 1) {
    return {
      ok: false,
      reason: 'ambiguous',
      candidates: [...new Set([...requestedMatches, ...desiredMatches])],
    };
  }

  if (desiredMatches.length === 0) {
    const englishMatches = findDocFileForRoute(context.docsRoot, contentRoute);
    return {
      ok: false,
      reason: docContext.locale !== 'en' && englishMatches.length > 0 ? 'locale-mismatch' : 'missing-page',
    };
  }

  if (requestedMatches.length === 0 && routeInfo.locale === docContext.locale) {
    return { ok: false, reason: 'missing-page' };
  }

  const desiredFile = desiredMatches[0];
  const requestedFile = requestedMatches[0] ?? findEnglishDoc(context.docsRoot, contentRoute);
  const anchor = resolveAnchor({
    requestedFile,
    desiredFile,
    englishFile: findEnglishDoc(context.docsRoot, contentRoute),
    fragment: parsed.fragment,
  });
  if (!anchor.ok) {
    return anchor;
  }

  return {
    ok: true,
    rewritten: composeTarget(fileToRoute(desiredFile, context.docsRoot), parsed.query, anchor.fragment),
  };
}

function fixRelativeTarget(parsed, file, context) {
  const fileDir = path.dirname(file);
  const directPath = path.resolve(fileDir, parsed.pathPart);
  const extension = path.extname(parsed.pathPart).toLowerCase();
  const assetLike = parsed.pathPart.includes('/assets/') || ASSET_EXTENSIONS.has(extension);

  if (assetLike) {
    const asset = resolveAssetFile(parsed.pathPart, directPath, context.assetsRoot);
    if (!asset.ok) {
      return asset;
    }
    return {
      ok: true,
      rewritten: composeTarget(relativePath(fileDir, asset.file), parsed.query, parsed.fragment),
    };
  }

  const directDocMatches = findDocFilesAtPath(directPath);
  let requestedFile = directDocMatches.length === 1 ? directDocMatches[0] : null;
  if (directDocMatches.length > 1) {
    return { ok: false, reason: 'ambiguous', candidates: directDocMatches };
  }

  const docContext = getDocContext(file, context.docsRoot);
  let contentRoute = requestedFile
    ? contentRouteForFile(requestedFile, context.docsRoot)
    : contentRouteFromBrokenRelativePath(parsed.pathPart, context.localeNames);

  if (!requestedFile && contentRoute !== null) {
    const requestedMatches = findDocFileForRoute(docContext.localeRoot, contentRoute);
    if (requestedMatches.length > 1) {
      return { ok: false, reason: 'ambiguous', candidates: requestedMatches };
    }
    requestedFile = requestedMatches[0] ?? null;
  }

  if (!requestedFile && isFile(directPath)) {
    return {
      ok: true,
      rewritten: composeTarget(relativePath(fileDir, directPath), parsed.query, parsed.fragment),
    };
  }

  if (!requestedFile || contentRoute === null) {
    return { ok: false, reason: 'missing-page' };
  }

  const desiredMatches = findDocFileForRoute(docContext.localeRoot, contentRoute);
  if (desiredMatches.length > 1) {
    return { ok: false, reason: 'ambiguous', candidates: desiredMatches };
  }
  if (desiredMatches.length === 0) {
    return { ok: false, reason: 'locale-mismatch' };
  }

  const desiredFile = desiredMatches[0];
  const anchor = resolveAnchor({
    requestedFile,
    desiredFile,
    englishFile: findEnglishDoc(context.docsRoot, contentRoute),
    fragment: parsed.fragment,
  });
  if (!anchor.ok) {
    return anchor;
  }

  return {
    ok: true,
    rewritten: composeTarget(relativePath(fileDir, desiredFile), parsed.query, anchor.fragment),
  };
}

function resolveAssetFile(rawPath, directPath, assetsRoot) {
  if (isInside(assetsRoot, directPath) && isFile(directPath)) {
    return { ok: true, file: directPath };
  }

  const normalized = normalizePosix(rawPath);
  const assetsMarker = '/assets/';
  const markerIndex = normalized.lastIndexOf(assetsMarker);
  if (markerIndex >= 0) {
    const assetRelative = normalized.slice(markerIndex + assetsMarker.length);
    const candidate = path.resolve(assetsRoot, assetRelative);
    if (isInside(assetsRoot, candidate) && isFile(candidate)) {
      return { ok: true, file: candidate };
    }
    return { ok: false, reason: 'missing-asset' };
  }

  const candidateMatches = [];
  const wanted = normalizePosix(stripLeadingTraversal(rawPath));
  for (const candidate of walkFiles(assetsRoot)) {
    const relative = normalizePosix(path.relative(assetsRoot, candidate));
    if (relative === wanted || relative.endsWith(`/${wanted}`)) {
      candidateMatches.push(candidate);
    }
  }

  if (candidateMatches.length === 1) {
    return { ok: true, file: candidateMatches[0] };
  }
  if (candidateMatches.length > 1) {
    return { ok: false, reason: 'ambiguous', candidates: candidateMatches };
  }
  return { ok: false, reason: 'missing-asset' };
}

function resolveAnchor({ requestedFile, desiredFile, englishFile, fragment }) {
  if (!fragment) {
    return { ok: true, fragment: '' };
  }

  const decoded = safeDecodeURIComponent(fragment);
  const desiredHeadings = readHeadings(desiredFile);
  if (desiredHeadings.some((heading) => heading.slug === decoded)) {
    return { ok: true, fragment: decoded };
  }

  for (const sourceFile of [requestedFile, englishFile]) {
    if (!sourceFile || !isFile(sourceFile)) {
      continue;
    }
    const sourceHeadings = readHeadings(sourceFile);
    const sourceIndex = sourceHeadings.findIndex((heading) => heading.slug === decoded);
    if (sourceIndex < 0) {
      continue;
    }
    const sameStructure =
      sourceHeadings.length === desiredHeadings.length &&
      sourceHeadings.every((heading, index) => heading.depth === desiredHeadings[index].depth);
    if (!sameStructure) {
      return { ok: false, reason: 'anchor-structure-mismatch' };
    }
    const desiredHeading = desiredHeadings[sourceIndex];
    return { ok: true, fragment: desiredHeading.slug };
  }

  return { ok: false, reason: 'missing-anchor' };
}

function readHeadings(file) {
  const text = fsSync.readFileSync(file, 'utf8');
  const headings = [];
  const slugger = new GithubSlugger();
  let inFence = false;
  let fenceMarker = null;

  for (const line of text.split('\n')) {
    const fenceMatch = line.match(/^(\s*)(`{3,}|~{3,})/);
    if (fenceMatch) {
      const marker = fenceMatch[2][0];
      if (!inFence) {
        inFence = true;
        fenceMarker = marker;
      } else if (marker === fenceMarker) {
        inFence = false;
        fenceMarker = null;
      }
      continue;
    }
    if (inFence) {
      continue;
    }

    const match = line.match(/^(#{1,6})\s+(.+?)\s*#*\s*$/);
    if (!match) {
      continue;
    }
    const explicitId = match[2].match(/\s*\{#([^}]+)\}\s*$/)?.[1];
    const headingText = match[2]
      .replace(/\s*\{#[^}]+\}\s*$/, '')
      .replace(/<[^>]+>/g, '')
      .replace(/!?(?:\[([^\]]*)\]\([^)]*\))/g, '$1')
      .replace(/[`*_~]/g, '');
    headings.push({
      depth: match[1].length,
      slug: explicitId ?? slugger.slug(headingText),
    });
  }

  return headings;
}

function splitTarget(target) {
  const hashIndex = target.indexOf('#');
  const beforeHash = hashIndex >= 0 ? target.slice(0, hashIndex) : target;
  const fragment = hashIndex >= 0 ? target.slice(hashIndex + 1) : '';
  const queryIndex = beforeHash.indexOf('?');
  return {
    pathPart: queryIndex >= 0 ? beforeHash.slice(0, queryIndex) : beforeHash,
    query: queryIndex >= 0 ? beforeHash.slice(queryIndex + 1) : '',
    fragment,
  };
}

function composeTarget(pathPart, query, fragment) {
  return `${pathPart}${query ? `?${query}` : ''}${fragment ? `#${encodeFragment(fragment)}` : ''}`;
}

function encodeFragment(fragment) {
  return fragment;
}

function safeDecodeURIComponent(value) {
  try {
    return decodeURIComponent(value);
  } catch {
    return value;
  }
}

function stripAngleBrackets(value) {
  if (value.startsWith('<') && value.endsWith('>')) {
    return value.slice(1, -1);
  }
  return value;
}

function stripWrappingQuotes(value) {
  if ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith("'") && value.endsWith("'"))) {
    return value.slice(1, -1);
  }
  return value;
}

function wrapIfQuoted(original, replacement) {
  const trimmed = original.trim();
  if ((trimmed.startsWith('"') && trimmed.endsWith('"')) || (trimmed.startsWith("'") && trimmed.endsWith("'"))) {
    return `${trimmed[0]}${replacement}${trimmed[0]}`;
  }
  return replacement;
}

function isExternalTarget(target) {
  return target.startsWith('//') || /^[a-z][a-z\d+.-]*:/i.test(target);
}

function normalizeServiceRoute(target) {
  return target.endsWith('/') ? target : `${target}/`;
}

function isDocsRoute(target) {
  return target === DOCS_BASE || target === `${DOCS_BASE}/` || target.startsWith(`${DOCS_BASE}/`);
}

function routeInsideDocsBase(target) {
  if (target === DOCS_BASE || target === `${DOCS_BASE}/`) {
    return '';
  }
  return normalizeRoutePath(target.slice(`${DOCS_BASE}/`.length));
}

function splitRouteLocale(route, knownLocales) {
  const parts = normalizeRoutePath(route).split('/').filter(Boolean);
  if (parts.length > 0 && knownLocales.has(parts[0])) {
    return {
      locale: parts[0],
      contentRoute: parts.slice(1).join('/'),
    };
  }
  return {
    locale: 'en',
    contentRoute: parts.join('/'),
  };
}

function discoverLocaleNames(root) {
  const locales = new Set();
  for (const entry of fsSync.readdirSync(root, { withFileTypes: true })) {
    if (!entry.isDirectory() || ENGLISH_DOC_SECTIONS.has(entry.name)) {
      continue;
    }
    if (findDocFileForRoute(path.join(root, entry.name), '').length > 0) {
      locales.add(entry.name);
    }
  }
  return locales;
}

function getDocLocale(file, docsRoot) {
  const relative = path.relative(docsRoot, file);
  const [first] = relative.split(path.sep);
  if (!first || first.startsWith('..')) {
    return 'en';
  }
  if (first === 'index.md' || first === 'index.mdx' || ENGLISH_DOC_SECTIONS.has(first)) {
    return 'en';
  }
  return first;
}

function getDocContext(file, docsRoot) {
  const locale = getDocLocale(file, docsRoot);
  return {
    locale,
    localeRoot: locale === 'en' ? docsRoot : path.join(docsRoot, locale),
  };
}

function findDocFileForRoute(root, route) {
  if (!isDirectory(root)) {
    return [];
  }
  const normalized = normalizeRoutePath(stripDocExtension(route));
  const base = normalized ? path.resolve(root, normalized) : root;
  if (!isInside(root, base)) {
    return [];
  }
  return uniqueExistingFiles([
    `${base}.md`,
    `${base}.mdx`,
    path.join(base, 'index.md'),
    path.join(base, 'index.mdx'),
  ]);
}

function findDocFilesAtPath(candidate) {
  const extension = path.extname(candidate).toLowerCase();
  if (FILE_EXTENSIONS.has(extension)) {
    return isFile(candidate) ? [candidate] : [];
  }
  return uniqueExistingFiles([
    candidate,
    `${candidate}.md`,
    `${candidate}.mdx`,
    path.join(candidate, 'index.md'),
    path.join(candidate, 'index.mdx'),
  ]).filter((file) => FILE_EXTENSIONS.has(path.extname(file).toLowerCase()));
}

function contentRouteForFile(file, docsRoot) {
  const locale = getDocLocale(file, docsRoot);
  const localeRoot = locale === 'en' ? docsRoot : path.join(docsRoot, locale);
  let relative = stripDocExtension(normalizePosix(path.relative(localeRoot, file)));
  if (relative === 'index') {
    return '';
  }
  if (relative.endsWith('/index')) {
    relative = relative.slice(0, -'/index'.length);
  }
  return normalizeRoutePath(relative);
}

function contentRouteFromBrokenRelativePath(rawPath, knownLocales) {
  const normalized = normalizeRoutePath(stripDocExtension(stripLeadingTraversal(rawPath)));
  const parts = normalized.split('/').filter(Boolean);
  if (parts.length === 0) {
    return '';
  }
  if (knownLocales.has(parts[0])) {
    parts.shift();
  }
  const sectionIndex = parts.findIndex((part) => ENGLISH_DOC_SECTIONS.has(part));
  if (sectionIndex >= 0) {
    return parts.slice(sectionIndex).join('/').replace(/\/index$/, '');
  }
  return null;
}

function findEnglishDoc(docsRoot, contentRoute) {
  const matches = findDocFileForRoute(docsRoot, contentRoute);
  return matches.length === 1 ? matches[0] : null;
}

function fileToRoute(file, docsRoot) {
  const locale = getDocLocale(file, docsRoot);
  const contentRoute = contentRouteForFile(file, docsRoot);
  const localePart = locale === 'en' ? '' : `${locale}/`;
  return `${DOCS_BASE}/${localePart}${contentRoute ? `${contentRoute}/` : ''}`;
}

function relativePath(fromDir, target) {
  const relative = normalizePosix(path.relative(fromDir, target));
  return relative || '.';
}

function normalizeRoutePath(value) {
  return normalizePosix(value).replace(/^\/+/, '').replace(/\/+$/, '');
}

function normalizePosix(value) {
  return value.split(path.sep).join('/');
}

function stripLeadingTraversal(value) {
  return normalizePosix(value).replace(/^(\.\.\/)+/, '').replace(/^(\.\/)+/, '');
}

function stripDocExtension(value) {
  return value.replace(/\.(md|mdx)$/i, '');
}

function uniqueExistingFiles(candidates) {
  return [...new Set(candidates.filter(isFile))];
}

function* walkFiles(root) {
  if (!isDirectory(root)) {
    return;
  }
  for (const entry of fsSync.readdirSync(root, { withFileTypes: true })) {
    const absolute = path.join(root, entry.name);
    if (entry.isDirectory()) {
      yield* walkFiles(absolute);
    } else {
      yield absolute;
    }
  }
}

function isFile(file) {
  try {
    return fsSync.statSync(file).isFile();
  } catch {
    return false;
  }
}

function isDirectory(directory) {
  try {
    return fsSync.statSync(directory).isDirectory();
  } catch {
    return false;
  }
}

function isInside(root, candidate) {
  const relative = path.relative(root, candidate);
  return relative === '' || (!relative.startsWith('..') && !path.isAbsolute(relative));
}

function relativeForOutput(file) {
  return normalizePosix(path.relative(repoRoot, file));
}

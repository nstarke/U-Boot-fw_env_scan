const { listBinaryEntries } = require('./shared');

module.exports = function registerRootRoute(app, deps) {
  const { testsDir, fsp, verboseRequestLog, verboseResponseLog } = deps;

  async function listTestEntries(dir, prefix = '') {
    const entries = await fsp.readdir(dir, { withFileTypes: true }).catch(() => []);
    const nested = await Promise.all(entries.map(async (entry) => {
      const relPath = prefix ? `${prefix}/${entry.name}` : entry.name;
      const fullPath = deps.path.join(dir, entry.name);
      if (entry.isDirectory()) {
        return listTestEntries(fullPath, relPath);
      }
      if (entry.isFile() && entry.name.endsWith('.sh')) {
        return [{
          name: relPath,
          url: `/tests/${encodeURIComponent(relPath).replace(/%2F/g, '/')}`
        }];
      }
      return [];
    }));

    return nested.flat().sort((a, b) => a.name.localeCompare(b.name));
  }

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  app.get('/', async (req, res) => {
    verboseRequestLog(req);
    const binaryEntries = await listBinaryEntries(deps.assetsDir, fsp, deps.releaseStateFile);
    const testEntries = await listTestEntries(testsDir);

    const assetItems = binaryEntries.length
      ? binaryEntries.map(({ fileName, url }) => `      <li><a href="${escapeHtml(url)}">${escapeHtml(fileName)}</a></li>`).join('\n')
      : '      <li><em>No binaries downloaded.</em></li>';

    const testItems = testEntries.length
      ? testEntries.map(({ name, url }) => `      <li><a href="${escapeHtml(url)}">tests/${escapeHtml(name)}</a></li>`).join('\n')
      : '      <li><em>No test shell scripts found.</em></li>';

    const html = `<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>Release Binaries and Test Scripts</title>
  </head>
  <body>
    <h1>Release Binaries</h1>
    <p>Serving files from: ${escapeHtml(deps.assetsDir)}</p>
    <ul>
${assetItems}
    </ul>

    <h1>Test Scripts</h1>
    <p>Serving scripts from: ${escapeHtml(testsDir)}</p>
    <ul>
${testItems}
    </ul>
  </body>
</html>
`;

    res.type('text/html').send(html);
    verboseResponseLog(req, 200, Buffer.byteLength(html));
  });
};
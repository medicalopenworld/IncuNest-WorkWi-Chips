import { readdirSync, readFileSync, statSync } from 'node:fs';
import { join } from 'node:path';

const root = process.cwd();
const jsonFiles = [];
const tomlFiles = [];

function walk(dir) {
  const entries = readdirSync(dir);
  for (const entry of entries) {
    if (entry.startsWith(".git")) continue;
    if (entry === "node_modules") continue;
    if (entry === ".next") continue;
    const fullPath = join(dir, entry);
    const st = statSync(fullPath);
    if (st.isDirectory()) {
      walk(fullPath);
      continue;
    }
    if (entry.endsWith('.json')) jsonFiles.push(fullPath);
    if (entry.endsWith('.toml')) tomlFiles.push(fullPath);
  }
}

function validateJson(file) {
  try {
    JSON.parse(readFileSync(file, 'utf8'));
    return null;
  } catch (err) {
    return `JSON inválido: ${file}\n${err.message}`;
  }
}

function validateToml(file) {
  try {
    const content = readFileSync(file, 'utf8');
    if (!content.trim().length) throw new Error('archivo vacío');
    if (!content.includes('[')) throw new Error('no parece TOML válido');
    return null;
  } catch (err) {
    return `TOML inválido: ${file}\n${err.message}`;
  }
}

walk(root);

const errors = [];
for (const file of jsonFiles) {
  const err = validateJson(file);
  if (err) errors.push(err);
}
for (const file of tomlFiles) {
  const err = validateToml(file);
  if (err) errors.push(err);
}

if (errors.length) {
  console.error(errors.join('\n\n'));
  process.exit(1);
}

console.log(`OK: ${jsonFiles.length} JSON y ${tomlFiles.length} TOML validados.`);

import { mkdirSync, readdirSync, statSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const root = process.cwd();
const chipsDir = join(root, "chips");

// (module (func (export "chip_init")))
const wasmBytes = Buffer.from([
  0x00, 0x61, 0x73, 0x6d, // \0asm
  0x01, 0x00, 0x00, 0x00, // version 1
  0x01, 0x04, 0x01, 0x60, 0x00, 0x00, // type section
  0x03, 0x02, 0x01, 0x00, // function section
  0x07, 0x0d, 0x01, 0x09, 0x63, 0x68, 0x69, 0x70, 0x5f, 0x69, 0x6e, 0x69, 0x74, 0x00, 0x00, // export section
  0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b // code section
]);

const chipFolders = readdirSync(chipsDir).filter((entry) => {
  const full = join(chipsDir, entry);
  return statSync(full).isDirectory();
});

for (const folder of chipFolders) {
  const base = join(chipsDir, folder, folder);
  mkdirSync(join(chipsDir, folder), { recursive: true });
  writeFileSync(`${base}.chip.wasm`, wasmBytes);
}

console.log(`Generated ${chipFolders.length} placeholder chip.wasm files.`);

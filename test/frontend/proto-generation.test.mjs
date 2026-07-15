import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";

const execFileAsync = promisify(execFile);
const repositoryDirectory = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  "../..",
);
const frontendDirectory = path.join(repositoryDirectory, "src/wwwroot_brpc");
const generator = path.join(frontendDirectory, "scripts/generate-proto.mjs");
const generated = path.join(frontendDirectory, "js/generated/oj-proto.js");
const declarations = path.join(frontendDirectory, "js/generated/oj-proto.d.ts");

async function generateAndHash() {
  await execFileAsync(process.execPath, [generator], { cwd: repositoryDirectory });
  return createHash("sha256")
    .update(await readFile(generated))
    .update(await readFile(declarations))
    .digest("hex");
}

test("protobuf browser generation is reproducible", async () => {
  const first = await generateAndHash();
  const second = await generateAndHash();
  assert.equal(first, second);
  assert.match(await readFile(declarations, "utf8"), /submissionId\?: \(\(Long\|string\)\|null\)/);
});

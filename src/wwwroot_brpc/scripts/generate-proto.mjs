import { execFile } from "node:child_process";
import { mkdir, readFile, readdir, rm, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";

import { build } from "esbuild";

const execFileAsync = promisify(execFile);
const scriptDirectory = path.dirname(fileURLToPath(import.meta.url));
const frontendDirectory = path.resolve(scriptDirectory, "..");
const repositoryDirectory = path.resolve(frontendDirectory, "../..");
const protoDirectory = path.join(repositoryDirectory, "src/comm/proto");
const outputDirectory = path.join(frontendDirectory, "js/generated");
const executableExtension = process.platform === "win32" ? ".cmd" : "";
const pbjs = path.join(frontendDirectory, "node_modules/.bin", `pbjs${executableExtension}`);
const pbts = path.join(frontendDirectory, "node_modules/.bin", `pbts${executableExtension}`);

const protoFiles = (await readdir(protoDirectory, { withFileTypes: true }))
  .filter((entry) => entry.isFile() && entry.name.endsWith(".proto"))
  .map((entry) => path.join(protoDirectory, entry.name))
  .sort();

if (protoFiles.length === 0) {
  throw new Error(`No .proto files found in ${protoDirectory}`);
}

await mkdir(outputDirectory, { recursive: true });
const temporaryDirectory = path.join(frontendDirectory, ".proto-build");
const staticModule = path.join(temporaryDirectory, "oj-proto.mjs");

try {
  await rm(temporaryDirectory, { recursive: true, force: true });
  await mkdir(temporaryDirectory, { recursive: true });
  await execFileAsync(pbjs, [
    "--target", "static-module",
    "--wrap", "es6",
    "--force-long",
    "--path", protoDirectory,
    "--out", staticModule,
    ...protoFiles,
  ]);

  await execFileAsync(pbts, [
    "--out", path.join(outputDirectory, "oj-proto.d.ts"),
    staticModule,
  ]);
  const declarationsPath = path.join(outputDirectory, "oj-proto.d.ts");
  const declarationLines = (await readFile(declarationsPath, "utf8")).split("\n");
  const browserDeclarations = declarationLines.map((line) => {
    if (line.startsWith("import Long =")) return line;
    return line
      .replaceAll("Long[]", "(Long|string)[]")
      .replace(/\bLong\b/g, "(Long|string)");
  }).join("\n");
  await writeFile(declarationsPath, browserDeclarations);

  await build({
    entryPoints: [staticModule],
    outfile: path.join(outputDirectory, "oj-proto.js"),
    bundle: true,
    format: "esm",
    nodePaths: [path.join(frontendDirectory, "node_modules")],
    platform: "browser",
    target: ["es2020"],
    legalComments: "none",
  });
} finally {
  await rm(temporaryDirectory, { recursive: true, force: true });
}

console.log(`Generated ${protoFiles.length} proto files into ${outputDirectory}`);

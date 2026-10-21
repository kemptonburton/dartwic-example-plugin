import assert from "node:assert/strict";
import test from "node:test";
import {validateStandalonePackageDependencies} from "./plugin-utils.mjs";

test("standalone source accepts registry and bundled dependencies", () => {
  assert.doesNotThrow(() => validateStandalonePackageDependencies({
    dependencies: {registry: "^1.0.0", bundled: "file:interface/sdk"},
  }));
});

test("standalone source rejects dependencies on the parent checkout", () => {
  for (const field of ["dependencies", "devDependencies", "optionalDependencies", "peerDependencies"]) {
    for (const spec of ["file:../..", "link:../dartwic", "../../dartwic"]) {
      assert.throws(() => validateStandalonePackageDependencies({[field]: {dartwic: spec}}), /outside this standalone repository/);
    }
  }
});

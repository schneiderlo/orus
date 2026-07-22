"""Bazel rule for an embedded build-facts provider backed by workspace status."""


def _embedded_build_facts_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".cc")
    ctx.actions.run_shell(
        inputs = [ctx.info_file],
        outputs = [output],
        arguments = [
            ctx.info_file.path,
            output.path,
            ctx.attr.product_version,
            ctx.attr.configuration,
            ctx.attr.compiler,
            ctx.attr.target_platform,
        ],
        command = """
set -eu
status_file="$1"
output="$2"
product_version="$3"
configuration="$4"
compiler="$5"
target_platform="$6"
source_revision="$(sed -n 's/^STABLE_ORUS_SOURCE_REVISION //p' "$status_file")"
test -n "$source_revision"
{
  printf '%s\n' '#include "orus/contracts/contracts.h"'
  printf '%s\n' ''
  printf '%s\n' 'namespace orus::contracts {'
  printf '%s\n' 'Result<JsonValue> EmbeddedBuildFacts() {'
  printf '  const BuildFacts facts{.product_version = "%s", .source_revision = "%s", .configuration = "%s", .compiler = "%s", .target_platform = "%s"};\n' "$product_version" "$source_revision" "$configuration" "$compiler" "$target_platform"
  printf '  return MakeBuildFacts(facts, %s);\n' "$([ "$configuration" = release ] && printf true || printf false)"
  printf '%s\n' '}'
  printf '%s\n' '}  // namespace orus::contracts'
} > "$output"
""",
        mnemonic = "OrusEmbeddedBuildFacts",
        progress_message = "Generating declared M0 build facts",
    )
    return [DefaultInfo(files = depset([output]))]


embedded_build_facts = rule(
    implementation = _embedded_build_facts_impl,
    attrs = {
        "compiler": attr.string(mandatory = True),
        "configuration": attr.string(mandatory = True),
        "product_version": attr.string(mandatory = True),
        "target_platform": attr.string(mandatory = True),
    },
)

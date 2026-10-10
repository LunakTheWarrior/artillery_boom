"""Select this application's metadata without changing the root .bazelrc."""

def _metadata_impl(settings, attr):
    return {"@pico-sdk//bazel/config:PICO_DEFAULT_BINARY_INFO": "//team_rocket_buzzer:binary_info"}

_metadata = transition(
    implementation = _metadata_impl,
    inputs = [],
    outputs = ["@pico-sdk//bazel/config:PICO_DEFAULT_BINARY_INFO"],
)

def _firmware_impl(ctx):
    source = ctx.attr.binary[0][DefaultInfo].files.to_list()[0]
    output = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.symlink(output = output, target_file = source)
    return [DefaultInfo(files = depset([output]))]

firmware = rule(
    implementation = _firmware_impl,
    attrs = {
        "binary": attr.label(mandatory = True, cfg = _metadata),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)

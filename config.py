def can_build(env, platform):
    if env["module_vorbis_enabled"]:
        env.module_add_dependencies("pxtone", ["vorbis", "ogg"])
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "AudioStreamPxTone",
    ]


def get_doc_path():
    return "doc_classes"

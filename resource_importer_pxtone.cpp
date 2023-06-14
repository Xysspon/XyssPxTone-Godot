/*************************************************************************/
/*  resource_importer_pxtone.cpp                                         */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/* Copyright (c) 2022-2023 Laura K. (alula)                              */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "resource_importer_pxtone.h"

#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "scene/resources/texture.h"

String ResourceImporterPxTone::get_importer_name() const {
	return "pxtone";
}

String ResourceImporterPxTone::get_visible_name() const {
	return "PxTone Collage";
}

void ResourceImporterPxTone::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("ptcop");
	p_extensions->push_back("pttune");
}

String ResourceImporterPxTone::get_save_extension() const {
	return "ptstr";
}

String ResourceImporterPxTone::get_resource_type() const {
	return "AudioStreamPxTone";
}

bool ResourceImporterPxTone::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	return true;
}

int ResourceImporterPxTone::get_preset_count() const {
	return 0;
}

String ResourceImporterPxTone::get_preset_name(int p_idx) const {
	return String();
}

void ResourceImporterPxTone::get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "loop"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "loop_offset"), 0));
}

Error ResourceImporterPxTone::import(const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	bool loop = p_options["loop"];
	float loop_offset = p_options["loop_offset"];

	Ref<FileAccess> f = FileAccess::open(p_source_file, FileAccess::READ);
	ERR_FAIL_COND_V(f.is_null(), ERR_CANT_OPEN);

	uint64_t len = f->get_length();

	Vector<uint8_t> data;
	data.resize(len);
	uint8_t *w = data.ptrw();

	f->get_buffer(w, len);

	Ref<AudioStreamPxTone> pxtn_stream;
	pxtn_stream.instantiate();

	pxtn_stream->set_data(data);
	ERR_FAIL_COND_V(!pxtn_stream->get_data().size(), ERR_FILE_CORRUPT);
	pxtn_stream->set_loop(loop);
	pxtn_stream->set_loop_offset(loop_offset);

	return ResourceSaver::save(pxtn_stream, p_save_path + ".ptstr");
}

ResourceImporterPxTone::ResourceImporterPxTone() {
}

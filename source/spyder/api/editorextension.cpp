#include "api/editorextension.h"

EditorExtension::EditorExtension()
{
	this->_enabled = false;
	this->_editor = nullptr;
	this->_on_close = false;
}

CodeEditor* EditorExtension::getEditor() const
{
	return this->_editor;
}

bool EditorExtension::getEnabled() const
{
	return this->_enabled;
}

void EditorExtension::setEnabled(bool enabled)
{
	if (enabled != this->_enabled)
	{
		this->_enabled = enabled;
		this->on_state_changed(enabled);
	}
}

void EditorExtension::on_install(CodeEditor* editor)
{
	this->_editor = editor;
	this->_enabled = true;
}

void EditorExtension::on_uninstall()
{
	this->_on_close = true;
	this->_enabled = false;
	this->_editor = nullptr;
}

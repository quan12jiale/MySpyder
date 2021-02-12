#pragma once

class CodeEditor;
class EditorExtension
{
public:
	EditorExtension();
	CodeEditor* getEditor() const;
	bool getEnabled() const;
	void setEnabled(bool enabled);
	virtual void on_install(CodeEditor* editor);
	virtual void on_uninstall();
	virtual void on_state_changed(bool state){}

protected:
	// name
	// description
	bool _enabled;
	CodeEditor* _editor;
	bool _on_close;
};
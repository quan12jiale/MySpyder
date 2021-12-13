#pragma once
#include "config/config_main.h"
#include <QTextDocument>

namespace sh {
	class BaseSH;
}

enum HighlighterEnumType
{
	TextSHType,
	PythonSHType,
	CppSHType,
	MarkdownSHType,
};

class ISyntaxHighlighterFactory
{
public:
	virtual ~ISyntaxHighlighterFactory() = default;
	virtual sh::BaseSH* createHighlighter(QTextDocument *parent, const QFont& font,
		const QHash<QString, ColorBoolBool>& color_scheme) = 0;
};

class HighlighterFactoryMgr
{
public:
	static HighlighterFactoryMgr* instance();
	void registerFactory(HighlighterEnumType type, ISyntaxHighlighterFactory* factory);
	sh::BaseSH* createHighlighter(HighlighterEnumType type, 
		QTextDocument *parent, 
		const QFont& font,
		const QHash<QString, ColorBoolBool>& color_scheme);

private:
	HighlighterFactoryMgr() = default;
	~HighlighterFactoryMgr() = default;

	QMap<HighlighterEnumType, ISyntaxHighlighterFactory*> m_mapHighlighterFactory;
};

template <HighlighterEnumType EnumType, typename ClassType>
class SyntaxHighlighterFactoryImpl : public ISyntaxHighlighterFactory
{
	// 1������ʱ���Լ�飺static_assert��
	// 2�������Եش����ؾ����Ƴ��������ػ�ģ���ػ���std::enable_if��
	// 3��#TODO ʹ�ÿɱ����ģ�壬����va_list
	// std::is_convertible_v<ClassType*, sh::BaseSH*>
	static_assert(std::is_base_of_v<sh::BaseSH, ClassType>,
		"ClassType must derived from sh::BaseSH");
public:
	SyntaxHighlighterFactoryImpl();

	sh::BaseSH* createHighlighter(QTextDocument *parent, const QFont& font,
		const QHash<QString, ColorBoolBool>& color_scheme) override
	{
		return new ClassType(parent, font, color_scheme);
	}
};

template <HighlighterEnumType EnumType, typename ClassType>
SyntaxHighlighterFactoryImpl<EnumType, ClassType>::SyntaxHighlighterFactoryImpl()
{
	HighlighterFactoryMgr::instance()->registerFactory(EnumType, this);
}

#define SPYDER_REGISTER_HIGHLIGHTER(enumType, classType) \
static SyntaxHighlighterFactoryImpl<enumType, classType> g_##enumType##classType;

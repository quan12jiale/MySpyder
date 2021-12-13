#include "utils/SyntaxHighlighterFactory.h"
#include "utils/syntaxhighlighters.h"

HighlighterFactoryMgr* HighlighterFactoryMgr::instance()
{
	static HighlighterFactoryMgr oMgr;
	return &oMgr;
}

void HighlighterFactoryMgr::registerFactory(HighlighterEnumType type, ISyntaxHighlighterFactory* factory)
{
	m_mapHighlighterFactory.insert(type, factory);
}

sh::BaseSH* HighlighterFactoryMgr::createHighlighter(HighlighterEnumType type,
	QTextDocument *parent,
	const QFont& font,
	const QHash<QString, ColorBoolBool>& color_scheme)
{
	if (m_mapHighlighterFactory.contains(type))
	{
		ISyntaxHighlighterFactory* factory = m_mapHighlighterFactory.value(type);
		return factory->createHighlighter(parent, font, color_scheme);
	}
	return nullptr;
}

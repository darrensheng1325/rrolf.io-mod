#include <Client/Ui/Text.hh>

#include <cassert>
#include <string>

#include <Client/Renderer.hh>

namespace app::ui
{
    Text::Text(Renderer &ctx)
        : Element(ctx),
          m_TextAlign(Renderer::TextAlign::Center),
          m_TextBaseLine(Renderer::TextBaseLine::Middle)
    {
    }

    Text::~Text()
    {
        assert(false); //oops
    }

    void Text::Render() const
    {
        //Guard g(m_Renderer);
        m_Renderer.SetTextAlign(m_TextAlign);
        m_Renderer.SetTextBaseLine(m_TextBaseLine);
        m_Renderer.SetTextSize(m_TextSize);
        m_Renderer.FillText(m_Text, m_X, m_Y);
    }
}
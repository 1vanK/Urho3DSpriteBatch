/*
    Аналог SpriteBatch (XNA, DirectXTK) и D3DXSprite.

    Использование:
    В функции Start():
    spriteBatch_ = new SpriteBatch(context_);

    В обработчике HandleEndViewRender:
    spriteBatch_->Begin();
    spriteBatch_->Draw(texture, Vector2(100, 100), nullptr, Color::WHITE, Vector2(20, 20), 180.0f);
    spriteBatch_->End();
    В этом случае SpriteBatch будет под интерфейсом.

    Если использовать E_ENDRENDERING, то SpriteBatch будет рисоваться поверх UI.
    Пригодится, например, для вывода курсора.

    Чтобы лучше понимать код, изучите https://github.com/1vanK/Urho3DTutor01 .
*/

#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/ShaderVariation.h>

using namespace Urho3D;

namespace Urho3D
{

class IndexBuffer;
class Font;
class Texture2D;
class Camera;
class VertexBuffer;

// Режимы зеркального отображения спрайтов.
enum SBEffects
{
    // Спрайт выводится как есть.
    SBE_NONE = 0,

    // Спрайт рисуется повернутым на 180 градусов вокруг вертикальной оси.
    SBE_FLIP_HORIZONTALLY = 1,

    // Спрайт рисуется повернутым на 180 градусов вокруг горизонтальной оси.
    SBE_FLIP_VERTICALLY = 2,

    // Комбинация из двух предыдущих режимов.
    SBE_FLIP_BOTH = SBE_FLIP_HORIZONTALLY | SBE_FLIP_VERTICALLY,
};

class URHO3D_API SpriteBatch : public Object
{
    URHO3D_OBJECT(SpriteBatch, Object);

public:
    // Размеры виртуального экрана. Если одна из координат <= 0, то используются
    // реальные размеры экрана.
    IntVector2 virtualScreenSize_ = IntVector2(0, 0);

    SpriteBatch(Context *context, unsigned maxPortionSize = 500);
    virtual ~SpriteBatch();

    // Если указать камеру, то SpriteBatch будет рендериться в мировых координатах.
    void Begin(BlendMode blendMode = BLEND_ALPHA, CompareMode compareMode = CMP_ALWAYS, float z = 0.0f, Camera* camera = nullptr);

    void End();

    void Draw(Texture2D* texture, const Rect& destination, Rect* source = nullptr, const Color& color = Color::WHITE,
        float rotation = 0.0f, const Vector2& origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, SBEffects effects = SBE_NONE);

    void Draw(Texture2D* texture, const Vector2& position, Rect* source = nullptr, const Color& color = Color::WHITE,
        float rotation = 0.0f, const Vector2 &origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, SBEffects effects = SBE_NONE);

    void DrawString(const String& text, Font* font, float fontSize, const Vector2& position, const Color& color = Color::WHITE,
        float rotation = 0.0f, const Vector2& origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, SBEffects effects = SBE_NONE);

    // Переводит реальные координаты в виртуальные. Используется для курсора мыши.
    Vector2 GetVirtualPos(const Vector2& realPos);

protected:
    // Отдельный спрайт в очереди на отрисовку.
    struct SBSprite
    {
        Texture2D* texture_;
        Rect destination_;
        Rect source_;
        Color color_;
        float rotation_;
        Vector2 origin_;
        Vector2 scale_;
        SBEffects effects_;

        // Для отрисовки текста и обычных спрайтов нужны разные шейдеры.
        ShaderVariation* vertexShader_;
        ShaderVariation* pixelShader_;
    };

    // Размер порции (максимальное число спрайтов, выводимых за один DrawCall).
    // Оптимальное значение СИЛЬНО зависит от конкретного оборудования и используемого API.
    unsigned maxPortionSize_;

    // Индексный буфер создается и заполняется один раз, а потом только используется.
    SharedPtr<IndexBuffer> indexBuffer_;

    SharedPtr<VertexBuffer> vertexBuffer_;

    // Спрайты, которые ожидают рендеринга.
    PODVector<SBSprite> sprites_;

    // Кэширование часто используемых вещей.
    Graphics* graphics_;
    ShaderVariation* spriteVS_;
    ShaderVariation* spritePS_;
    ShaderVariation* ttfTextVS_;
    ShaderVariation* ttfTextPS_;
    ShaderVariation* spriteTextVS_;
    ShaderVariation* spriteTextPS_;
    ShaderVariation* sdfTextVS_;
    ShaderVariation* sdfTextPS_;

    // Режим наложения.
    BlendMode blendMode_;

    // Если использовать CMP_LESSEQUAL, то модели, которые ближе к камере,
    // не будут перекрыты спрайтами (учитывается буфер глубины).
    CompareMode compareMode_;

    float z_;

    // Если определена камера, то SpriteBatch рендерится в мировых координатах.
    Camera* camera_;

    // Это значение вычисляется в функции Begin().
    IntRect viewportRect_;

    // Рендерит порцию спрайтов, использующих одну и ту же текстуру и шейдер.
    void RenderPortion(unsigned start, unsigned count);

    // Определяет количество спрайтов, которые можно отренедерить без
    // смены текстуры и шейдера.
    unsigned GetPortionLength(unsigned start);

    // Если определена камера, то спрайты будут отрендерены в мировых координатах,
    // иначе - в экранных.
    Matrix4 GetViewProjMatrix();
};

}
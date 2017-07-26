#include "SpriteBatch.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/FontFace.h>

// Спрайт состоит из двух треугольников, а значит у него 6 вершин.
// То есть каждый спрайт занимает 6 элементов в индексном буфере.
#define INDICES_PER_SPRITE 6

// Две вершины спрайта идентичны для обоих треугольников, поэтому
// в вершинном буфере каждый спрайт занимает 4 элемента.
#define VERTICES_PER_SPRITE 4

namespace Urho3D
{

// Атрибуты вершин.
struct SBVertex
{
    Vector3 position_;
    unsigned color_;
    Vector2 uv_;
};

SpriteBatch::SpriteBatch(Context *context, unsigned maxPortionSize) :
    Object(context),
    maxPortionSize_(maxPortionSize),
    indexBuffer_(new IndexBuffer(context_)),
    vertexBuffer_(new VertexBuffer(context_))
{
    // Индексный буфер дублируется в памяти CPU и автоматически восстанавливается
    // при потере устройства.
    indexBuffer_->SetShadowed(true);

    // Индексный буфер никогда не меняется, поэтому мы можем его сразу заполнить.
    indexBuffer_->SetSize(maxPortionSize_ * INDICES_PER_SPRITE, false);
    unsigned short* buffer = (unsigned short*)indexBuffer_->Lock(0, indexBuffer_->GetIndexCount());
    for (unsigned i = 0; i < maxPortionSize_; i++)
    {
        // Первый треугольник спрайта.
        buffer[i * INDICES_PER_SPRITE + 0] = i * VERTICES_PER_SPRITE + 0;
        buffer[i * INDICES_PER_SPRITE + 1] = i * VERTICES_PER_SPRITE + 1;
        buffer[i * INDICES_PER_SPRITE + 2] = i * VERTICES_PER_SPRITE + 2;
        
        // Второй треугольник спрайта.
        buffer[i * INDICES_PER_SPRITE + 3] = i * VERTICES_PER_SPRITE + 2;
        buffer[i * INDICES_PER_SPRITE + 4] = i * VERTICES_PER_SPRITE + 3;
        buffer[i * INDICES_PER_SPRITE + 5] = i * VERTICES_PER_SPRITE + 0;
    }
    indexBuffer_->Unlock();

    vertexBuffer_->SetSize(maxPortionSize_ * VERTICES_PER_SPRITE,
                           MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1, true);

    graphics_ = GetSubsystem<Graphics>();
    spriteVS_ = graphics_->GetShader(VS, "Basic", "DIFFMAP VERTEXCOLOR");
    spritePS_ = graphics_->GetShader(PS, "Basic", "DIFFMAP VERTEXCOLOR");
    ttfTextVS_ = graphics_->GetShader(VS, "Text");
    ttfTextPS_ = graphics_->GetShader(PS, "Text", "ALPHAMAP");
    spriteTextVS_ = graphics_->GetShader(VS, "Text");
    spriteTextPS_ = graphics_->GetShader(PS, "Text");
    sdfTextVS_ = graphics_->GetShader(VS, "Text");
    sdfTextPS_ = graphics_->GetShader(PS, "Text", "SIGNED_DISTANCE_FIELD");
}

SpriteBatch::~SpriteBatch()
{
}

void SpriteBatch::Begin(BlendMode blendMode, CompareMode compareMode, float z, Camera* camera)
{
    blendMode_ = blendMode;
    compareMode_ = compareMode;
    z_ = z;
    camera_ = camera;

    sprites_.Clear();

    // Вычисляем viewportRect_.
    if (virtualScreenSize_.x_ <= 0 || virtualScreenSize_.y_ <= 0)
    {
        // Виртуальный экран не используется. Вьюпорт занимает все окно.
        viewportRect_ = IntRect(0, 0, graphics_->GetWidth(), graphics_->GetHeight());
    }
    else
    {
        float realAspect = (float)graphics_->GetWidth() / graphics_->GetHeight();
        float virtualAspect = (float)virtualScreenSize_.x_ / virtualScreenSize_.y_;

        float virtualScreenScale;
        if (realAspect > virtualAspect)
        {
            // Окно шире, чем надо. Будут черные полосы по бокам.
            virtualScreenScale = (float)graphics_->GetHeight() / virtualScreenSize_.y_;
        }
        else
        {
            // Высота окна больше, чем надо. Будут черные полосы сверху и снизу.
            virtualScreenScale = (float)graphics_->GetWidth() / virtualScreenSize_.x_;
        }

        int viewportWidth = (int)(virtualScreenSize_.x_ * virtualScreenScale);
        int viewportHeight = (int)(virtualScreenSize_.y_ * virtualScreenScale);

        // Центрируем вьюпорт.
        int viewportX = (graphics_->GetWidth() - viewportWidth) / 2;
        int viewportY = (graphics_->GetHeight() - viewportHeight) / 2;

        viewportRect_ = IntRect(viewportX, viewportY, viewportWidth + viewportX, viewportHeight + viewportY);
    }
}

void SpriteBatch::Draw(Texture2D* texture, const Rect& destination, Rect* source,
    const Color& color, float rotation, const Vector2& origin, const Vector2& scale, SBEffects effects)
{
    SBSprite sprite
    {
        texture,
        destination,
        source ? *source : Rect(0.0f, 0.0f, (float)texture->GetWidth(), (float)texture->GetHeight()),
        color,
        rotation,
        origin,
        scale,
        effects,
        spriteVS_,
        spritePS_
    };

    sprites_.Push(sprite);
}

void SpriteBatch::Draw(Texture2D* texture, const Vector2& position, Rect* source,
    const Color& color, float rotation, const Vector2& origin, const Vector2& scale, SBEffects effects)
{
    Rect destination
    {
        position.x_,
        position.y_,
        position.x_ + texture->GetWidth(),
        position.y_ + texture->GetHeight()
    };

    Draw(texture, destination, source, color, rotation, origin, scale, effects);
}

void SpriteBatch::DrawString(const String& text, Font* font, float fontSize, const Vector2& position,
    const Color& color, float rotation, const Vector2& origin, const Vector2& scale, SBEffects effects)
{
    PODVector<unsigned> unicodeText;
    for (unsigned i = 0; i < text.Length();)
        unicodeText.Push(text.NextUTF8Char(i));

    FontFace* face = font->GetFace(fontSize);
    Vector2 pos = position;
    Vector2 charOrig = origin;

    unsigned i = 0;
    int step = 1;

    if (effects & SBE_FLIP_HORIZONTALLY)
    {
        i = unicodeText.Size() - 1;
        step = -1;
    }

    float sin, cos;
    SinCos(rotation, sin, cos);

    for (; i < unicodeText.Size(); i += step)
    {
        const FontGlyph* glyph = face->GetGlyph(unicodeText[i]);
        float gx = (float)glyph->x_;
        float gy = (float)glyph->y_;
        float gw = (float)glyph->width_;
        float gh = (float)glyph->height_;
        float gox = (float)glyph->offsetX_;
        float goy = (float)glyph->offsetY_;

        ShaderVariation* ps;
        ShaderVariation* vs;

        if (font->GetFontType() == FONT_FREETYPE)
        {
            ps = ttfTextPS_;
            vs = ttfTextVS_;
        }
        else // FONT_BITMAP
        {
            if (font->IsSDFFont())
            {
                ps = sdfTextPS_;
                vs = sdfTextVS_;
            }
            else
            {
                ps = spriteTextPS_;
                vs = spriteTextVS_;
            }
        }

        SBSprite sprite
        {
            face->GetTextures()[glyph->page_],
            Rect(position.x_, position.y_, position.x_ + gw, position.y_ + gh),
            Rect(gx, gy, gx + gw, gy + gh),
            color,
            rotation,
            (effects & SBE_FLIP_VERTICALLY) ? charOrig - Vector2(gox, 0.0f) : charOrig - Vector2(gox, goy),
            scale,
            effects,
            vs,
            ps
        };

        sprites_.Push(sprite);
        charOrig.x_ -= (float)glyph->advanceX_;
    }
}

void SpriteBatch::End()
{
    // Список спрайтов пуст.
    if (sprites_.Size() == 0)
        return;

    graphics_->ResetRenderTargets();
    graphics_->ClearParameterSources();
    graphics_->SetCullMode(CULL_NONE);
    graphics_->SetDepthTest(compareMode_);
    graphics_->SetBlendMode(blendMode_);
    graphics_->SetDepthWrite(false);
    graphics_->SetStencilTest(false);
    graphics_->SetScissorTest(false);
    graphics_->SetColorWrite(true);
    graphics_->SetIndexBuffer(indexBuffer_);
    graphics_->SetVertexBuffer(vertexBuffer_);
    graphics_->SetViewport(viewportRect_);

    unsigned startSpriteIndex = 0;
    while (startSpriteIndex != sprites_.Size())
    {
        unsigned count = GetPortionLength(startSpriteIndex);
        RenderPortion(startSpriteIndex, count);
        startSpriteIndex += count;
    }
}

Vector2 SpriteBatch::GetVirtualPos(const Vector2& realPos)
{
    if (virtualScreenSize_.x_ <= 0 || virtualScreenSize_.x_ <= 0)
        return realPos;

    float factor = (float)virtualScreenSize_.x_ / viewportRect_.Width();

    float virtualX = (realPos.x_ - viewportRect_.left_) * factor;
    float virtualY = (realPos.y_ - viewportRect_.top_) * factor;

    return Vector2(virtualX, virtualY);
}

Matrix4 SpriteBatch::GetViewProjMatrix()
{
    if (camera_)
        return camera_->GetGPUProjection() * camera_->GetView();

    int w = virtualScreenSize_.x_;
    int h = virtualScreenSize_.y_;

    // Размеры виртуального экрана не заданы.
    if (w <= 0 || h <= 0)
    {
        w = graphics_->GetWidth();
        h = graphics_->GetHeight();
    }

    // В DirectX 9 вершины нужно смещать на пол пикселя
    // http://drilian.com/2008/11/25/understanding-half-pixel-and-half-texel-offsets/
    float pixelWidth  = 2.0f / w; // Двойка так как длина отрезка [-1, 1] равна двум.
    float pixelHeight = 2.0f / h; // Подробности: https://github.com/1vanK/Urho3DTutor01 .
    Vector2 offset = graphics_->GetPixelUVOffset(); // Возвращает (0.5f, 0.5f) для DirectX 9 и (0.0f, 0.0f) в остальных случаях.
    offset.x_ *= pixelWidth;
    offset.y_ *= pixelHeight;

    return Matrix4(2.0f / w,    0.0f,         0.0f,   -1.0f - offset.x_,
                   0.0f,       -2.0f / h,     0.0f,    1.0f + offset.y_,
                   0.0f,        0.0f,         1.0f,    0.0f,
                   0.0f,        0.0f,         0.0f,    1.0f);
}

unsigned SpriteBatch::GetPortionLength(unsigned start)
{
    unsigned count = 1;

    while (true)
    {
        if (count >= maxPortionSize_)
            break;

        unsigned nextSpriteIndex = start + count;
        
        // Достигнут конец списка.
        if (nextSpriteIndex == sprites_.Size())
            break;
        
        // У следующего спрайта другая текстура.
        if (sprites_[nextSpriteIndex].texture_ != sprites_[start].texture_)
            break;

        if (sprites_[nextSpriteIndex].vertexShader_ != sprites_[start].vertexShader_)
            break;

        if (sprites_[nextSpriteIndex].pixelShader_ != sprites_[start].pixelShader_)
            break;

        count++;
    }

    return count;
}

// Никакие проверки не производятся, все входные данные должны быть корректными.
void SpriteBatch::RenderPortion(unsigned start, unsigned count)
{
    graphics_->SetShaders(sprites_[start].vertexShader_, sprites_[start].pixelShader_);
    if (graphics_->NeedParameterUpdate(SP_OBJECT, this))
        graphics_->SetShaderParameter(VSP_MODEL, Matrix3x4::IDENTITY);
    if (graphics_->NeedParameterUpdate(SP_CAMERA, this))
        graphics_->SetShaderParameter(VSP_VIEWPROJ, GetViewProjMatrix());
    if (graphics_->NeedParameterUpdate(SP_MATERIAL, this))
        graphics_->SetShaderParameter(PSP_MATDIFFCOLOR, Color(1.0f, 1.0f, 1.0f, 1.0f));

    Texture2D* texture = sprites_[start].texture_;
    
    SBVertex* vertices = (SBVertex*)vertexBuffer_->Lock(0, count * VERTICES_PER_SPRITE, true);
    float invw = 1.0f / texture->GetWidth();
    float invh = 1.0f / texture->GetHeight();
    for (unsigned i = 0; i < count; i++)
    {
        SBSprite* sprite = &sprites_[i + start];
        unsigned color = sprite->color_.ToUInt();
        Rect dest = sprite->destination_;
        Rect src = sprite->source_;
        Vector2 origin = sprite->origin_;
        SBEffects effects = sprite->effects_;
        Vector2 scale = sprite->scale_;

        // Если спрайт не повернут и не отмаcштабирован, то прорисовка очень проста.
        if (sprite->rotation_ == 0.0f && sprite->scale_ == Vector2::ONE)
        {
            // Сдвигаем спрайт на -origin.
            dest.min_ -= origin;
            dest.max_ -= origin;

            // Лицевая грань задается по часовой стрелке. Учитываем, что ось Y направлена вниз.
            // Но нет большой разницы, так как спрайты двусторонние.
            vertices[i * VERTICES_PER_SPRITE + 0].position_ = Vector3(dest.min_.x_, dest.min_.y_, z_); // Верхний левый угол спрайта.
            vertices[i * VERTICES_PER_SPRITE + 1].position_ = Vector3(dest.max_.x_, dest.min_.y_, z_); // Правый верхний угол.
            vertices[i * VERTICES_PER_SPRITE + 2].position_ = Vector3(dest.max_.x_, dest.max_.y_, z_); // Нижний правый угол.
            vertices[i * VERTICES_PER_SPRITE + 3].position_ = Vector3(dest.min_.x_, dest.max_.y_, z_); // Левый нижний угол.
        }
        else
        {
            // Если есть угол поворота, то определяем локальные координаты вершин спрайта,
            // то есть сдвигаем все вершины целевого спрайта на координату верхнего левого угла, чтобы
            // она находилась в начале координат (origin по умолчанию), а потом еще на указанный origin.
            Rect local(-origin, dest.max_ - dest.min_ - origin);

            // Матрица масштабирует и поворачивает вершину в локальных координатах, а затем
            // смещает ее в требуемые мировые координаты.
            float sin, cos;
            SinCos(sprite->rotation_, sin, cos);
            Matrix3 transform
            {
                cos * scale.x_, -sin * scale.y_,  dest.min_.x_,
                sin * scale.x_,  cos * scale.y_,  dest.min_.y_,
                0.0f,            0.0f,            1.0f
            };
            // Пробовал объединить смещение на -origin и трансформацию в одну матрицу, но потерял несколько ФПС из-за кучи дополнительных
            // умножений при составлении матрицы.

            // В движке вектор умножается на матрицу справа (в отличие от шейдеров). Вычисления в однородных координатах.
            Vector3 v0(local.min_.x_, local.min_.y_, 1.0f); // Верхний левый угол спрайта.
            v0 = transform * v0;
            v0.z_ = z_;
            vertices[i * VERTICES_PER_SPRITE + 0].position_ = v0;

            Vector3 v1(local.max_.x_, local.min_.y_, 1.0f); // Правый верхний угол.
            v1 = transform * v1;
            v1.z_ = z_;
            vertices[i * VERTICES_PER_SPRITE + 1].position_ = v1;

            Vector3 v2(local.max_.x_, local.max_.y_, 1.0f); // Нижний правый угол.
            v2 = transform * v2;
            v2.z_ = z_;
            vertices[i * VERTICES_PER_SPRITE + 2].position_ = v2;

            Vector3 v3(local.min_.x_, local.max_.y_, 1.0f); // Левый нижний угол.
            v3 = transform * v3;
            v3.z_ = z_;
            vertices[i * VERTICES_PER_SPRITE + 3].position_ = v3;
        }

        vertices[i * VERTICES_PER_SPRITE + 0].color_ = color;
        vertices[i * VERTICES_PER_SPRITE + 1].color_ = color;
        vertices[i * VERTICES_PER_SPRITE + 2].color_ = color;
        vertices[i * VERTICES_PER_SPRITE + 3].color_ = color;

        if (effects & SBE_FLIP_HORIZONTALLY)
        {
            float tmp = src.min_.x_;
            src.min_.x_ = src.max_.x_;
            src.max_.x_ = tmp;
        }

        if (effects & SBE_FLIP_VERTICALLY)
        {
            float tmp = src.min_.y_;
            src.min_.y_ = src.max_.y_;
            src.max_.y_ = tmp;
        }

        vertices[i * VERTICES_PER_SPRITE + 0].uv_ = Vector2(src.min_.x_ * invw, src.min_.y_ * invh);
        vertices[i * VERTICES_PER_SPRITE + 1].uv_ = Vector2(src.max_.x_ * invw, src.min_.y_ * invh);
        vertices[i * VERTICES_PER_SPRITE + 2].uv_ = Vector2(src.max_.x_ * invw, src.max_.y_ * invh);
        vertices[i * VERTICES_PER_SPRITE + 3].uv_ = Vector2(src.min_.x_ * invw, src.max_.y_ * invh);
    }
    vertexBuffer_->Unlock();
    
    graphics_->SetTexture(0, texture);
    graphics_->Draw(TRIANGLE_LIST, 0, count * INDICES_PER_SPRITE, 0, count * VERTICES_PER_SPRITE);
}

}
#include "halley/graphics/render_target/render_target_texture.h"
#include <gsl/assert>
#include "halley/graphics/texture.h"

using namespace Halley;

String TextureRenderTarget::getName() const
{
	String name = "Tex";
	if (!colourBuffer.empty() && colourBuffer[0]) {
		name += ":" + colourBuffer[0]->getAssetId();
	}
	return name;
}

void TextureRenderTarget::setTarget(int attachmentNumber, std::shared_ptr<Texture> tex)
{
	Expects(attachmentNumber >= 0);
	Expects(attachmentNumber < 8);
	Expects(tex);

	if (int(colourBuffer.size()) <= attachmentNumber) {
		colourBuffer.resize(size_t(attachmentNumber) + 1);
		dirty = true;
	}
	if (colourBuffer[attachmentNumber] != tex) {
		colourBuffer[attachmentNumber] = std::move(tex);
		dirty = true;
	}
}

const std::shared_ptr<Texture>& TextureRenderTarget::getTexture(int attachmentNumber) const
{
	return colourBuffer.at(attachmentNumber);
}

void TextureRenderTarget::setDepthTexture(std::shared_ptr<Texture> tex)
{
	if (tex != depthStencilBuffer) {
		depthStencilBuffer = std::move(tex);
		dirty = true;
	}
}

const std::shared_ptr<Texture>& TextureRenderTarget::getDepthTexture() const
{
	return depthStencilBuffer;
}

Rect4i TextureRenderTarget::getViewPort() const
{
	return viewPort ? viewPort.value() : Rect4i(Vector2i(0, 0), colourBuffer.empty() ? Vector2i() : getTexture(0)->getSize());
}

void TextureRenderTarget::setViewPort(Rect4i vp)
{
	viewPort = vp;
}

void TextureRenderTarget::resetViewPort()
{
	viewPort = std::optional<Rect4i>();
}

bool TextureRenderTarget::hasColourBuffer(int attachmentNumber) const
{
	return colourBuffer.size() > static_cast<size_t>(attachmentNumber);
}

bool TextureRenderTarget::hasDepthBuffer() const
{
	return !!depthStencilBuffer;
}

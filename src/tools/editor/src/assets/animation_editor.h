#pragma once

#include "halley/graphics/texture.h"
#include "halley/graphics/sprite/animation.h"
#include "halley/resources/resources.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_animation.h"
#include "asset_editor.h"
#include "src/ui/scroll_background.h"

namespace Halley {
	class Project;
	class AnimationEditorDisplay;

	class AnimationEditor : public AssetEditor {
    public:
        AnimationEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, MetadataEditor& metadataEditor);

        void refresh();
        void onResourceLoaded() override;
        void refreshAssets() override;

		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;
		bool onKeyPress(KeyboardKeyPress key) override;
	
    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;
		
	private:
		MetadataEditor& metadataEditor;

		void setupWindow();
		void loadAssetData();

		void togglePlay();
		void updatePlayIcon();
		void updateActionPointList();

        std::shared_ptr<AnimationEditorDisplay> animationDisplay;
        std::shared_ptr<UILabel> info;
        std::shared_ptr<ScrollBackground> scrollBg;
	};

	class AnimationEditorDisplay : public UIWidget {
	public:
		AnimationEditorDisplay(String id, Resources& resources);

		void setZoom(float zoom);
		void setAnimation(std::shared_ptr<const Animation> animation);
		void setSprite(std::shared_ptr<const SpriteResource> sprite);
		void setTexture(std::shared_ptr<const Texture> texture);
		void setSequence(const String& sequence);
		void setDirection(const String& direction);

		void refresh();

		const Rect4f& getBounds() const;
		Vector2f getMousePos() const;
		void onMouseOver(Vector2f mousePos, KeyMods keyMods) override;

		void setMetadataEditor(MetadataEditor& metadataEditor);

		bool isPlaying() const;
		void setPlaying(bool play);
		void nextFrame();
		void prevFrame();
		int getFrameNumber() const;

		void onDoubleClick();
		void setActionPoint(const String& pointId);
		void clearPoint();

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
		
	private:
		Resources& resources;
		std::shared_ptr<const Animation> animation;
		AnimationPlayer animationPlayer;
		MetadataEditor* metadataEditor = nullptr;

		Sprite origSprite;
		Sprite drawSprite;
		Sprite boundsSprite;
		Sprite nineSliceVSprite;
		Sprite nineSliceHSprite;
		Sprite actionPointSprite;

		std::optional<Vector2i> origPivot;
		Rect4i origBounds;
		Rect4f bounds;
		String actionPointId;

		float zoom = 1.0f;
		bool isoMode = false;
		Vector2f mousePos;
		Vector2f screenSpaceMousePos;

		void updateBounds();
		Vector2f imageToScreenSpace(Vector2f pos) const;
		Vector2f screenToImageSpace(Vector2f pos) const;
		std::optional<Vector4f> getCurrentSlices() const;

		Vector2i getCurrentPivot() const;
		std::optional<Vector2i> getCurrentActionPoint() const;
		void setCurrentActionPoint(std::optional<Vector2i> pos);

		int getMetaIntOr(const String& key, int defaultValue) const;
	};
}


#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class UThesisGameSubsystem;
struct FThesisEnemy;

class SThesisHudPainter : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SThesisHudPainter) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UThesisGameSubsystem>, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	struct FPaintContext
	{
		const FGeometry& Geometry;
		FSlateWindowElementList& Elements;
		int32 Layer;
	};

	void Box(FPaintContext& Context, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color, const FSlateBrush* Brush) const;
	FVector2D Text(FPaintContext& Context, const FString& Value, const FVector2D& Position, const FSlateFontInfo& Font, const FLinearColor& Color, bool bShadow = true) const;
	FVector2D Measure(const FString& Value, const FSlateFontInfo& Font) const;
	void Bar(FPaintContext& Context, const FVector2D& Position, const FVector2D& Size, double Fraction, const FLinearColor& FillColor) const;

	void PaintEnemyLabels(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FVector2D& LocalSize) const;
	void PaintEnemyLabel(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FThesisEnemy& Enemy, const FVector2D& Anchor, float Scale) const;
	void PaintPlayerPanel(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FVector2D& LocalSize) const;
	void PaintObjective(FPaintContext& Context, const UThesisGameSubsystem& Subsystem) const;
	void PaintStats(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FVector2D& LocalSize) const;
	void PaintHints(FPaintContext& Context, const FVector2D& LocalSize) const;
	void PaintToasts(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FVector2D& LocalSize) const;
	void PaintHitMarker(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FVector2D& LocalSize) const;
	void PaintDamageVignette(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FVector2D& LocalSize) const;
	void PaintDebugPanel(FPaintContext& Context, const UThesisGameSubsystem& Subsystem, const FVector2D& LocalSize) const;

	TWeakObjectPtr<UThesisGameSubsystem> Subsystem;
};

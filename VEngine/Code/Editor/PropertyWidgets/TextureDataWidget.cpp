#include "vpch.h"
#include "TextureDataWidget.h"
#include <qfiledialog.h>
#include "Render/RenderTypes.h"
#include "VString.h"
#include "Asset/AssetPaths.h"

TextureDataWidget::TextureDataWidget(Property& prop_)
{
	prop = prop_;
	value = (TextureData*)prop.data;

	setText(QString::fromStdString(value->filename));
	connect(this, &QPushButton::clicked, this, &TextureDataWidget::SetValue);
}

void TextureDataWidget::SetValue()
{
	//Note: You have to call CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) for the native file
	//dialog to work properly.
	QString filepath = QFileDialog::getOpenFileName(this,
		tr("Set Texture"),
		QString::fromStdString(AssetBaseFolders::texture),
		tr("Image Files (*.png *.jpg *.bmp)"));

	QFileInfo info(filepath);
	QString filename = info.fileName();

	if (!filename.isEmpty())
	{
		value->filename.assign(
			VString::GetSubStringAtFoundOffset(filepath.toStdString(), AssetBaseFolders::texture));

		setToolTip(filepath);

		if (prop.change)
		{
			prop.change(value);
		}

		ResetValue();
	}
}

void TextureDataWidget::ResetValue()
{
	if (value)
	{
		setText(QString::fromStdString(value->filename));
	}
}

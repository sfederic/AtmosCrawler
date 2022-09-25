#include "vpch.h"
#include "StringWidget.h"
#include "Properties.h"
#include <QDir>
#include <filesystem>
#include <QDirIterator>
#include <QStringListModel>
#include <QCompleter>

StringWidget::StringWidget(Property& value_)
{
	prop = value_;

	value = value_.GetData<std::string>();
	setText(QString::fromStdString(value->data()));

	SetAutoComplete(prop.autoCompletePath);

	connect(this, &QLineEdit::editingFinished, this, &StringWidget::SetValue);
}

void StringWidget::SetValue()
{
	QString txt = text();
	value->assign(txt.toStdString().c_str());

	if (prop.change)
	{
		prop.change(value);
	}

	clearFocus();
}

void StringWidget::ResetValue()
{
	if (value)
	{
		setText(QString::fromStdString(value->data()));
	}
}

void StringWidget::SetAutoComplete(const std::string& directoryPath)
{
	if (directoryPath.empty()) return;

	std::string path = std::filesystem::current_path().generic_string() + prop.autoCompletePath;

	QStringList dirContents;

	for (auto const& entry : std::filesystem::recursive_directory_iterator(path))
	{
		//Use generic_string() here. Windows likes to throw in '\\' when it wants with string().
		std::string path = entry.path().generic_string();

		//Grab the index so filepaths are displayed correctly on autocomplete. Eg. "test_map/item.png"
		size_t index = path.find(prop.autoCompletePath);
		std::string file = path.substr(index + prop.autoCompletePath.size());

		dirContents.append(QString::fromStdString(file));
	}

	QCompleter* fileEditCompleter = new QCompleter(dirContents, this);
	fileEditCompleter->setCaseSensitivity(Qt::CaseInsensitive);
	fileEditCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	this->setCompleter(fileEditCompleter);
}

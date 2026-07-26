// Ported from FModel/Views/Resources/Controls/EndpointEditor.xaml.cs
#include "EndpointEditor.h"

#include <QCheckBox>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../Settings/EndpointSettings.h"
#include "../../../ViewModels/Commands/MenuCommand.h"

namespace FModel::Views::Resources::Controls
{
    using Settings::EndpointSettings;

    const QString EndpointEditor::SyntaxLink =
        QStringLiteral("https://support.smartbear.com/alertsite/docs/monitors/api/endpoint/jsonpath.html");
    const QString EndpointEditor::EvaluatorLink = QStringLiteral("https://jsonpath.com/");

    QString EndpointEditor::instructions(EEndpointType type)
    {
        switch (type)
        {
            case EEndpointType::Aes:
                return QStringLiteral(
                    "In order to make this work, you first need to understand JSON and its query language. If "
                    "you don't, please close this window. If your game never changes its AES keys or is not "
                    "even encrypted, please close this window. If you do understand what you are doing, you "
                    "have to know that the AES expression supports up to 2 elements.\n"
                    "\n"
                    "    The first element is mandatory and will be assigned to the main AES key. It has to be "
                    "looking like a key, else your configuration will not be valid (the key validity against "
                    "your files will not be checked). Said key must be hexadecimal and can start without "
                    "\"0x\".\n"
                    "\n"
                    "    If your game uses several AES keys, you can specify a second element that will be your "
                    "list of dynamic keys. The format needed is a list of objects with, at least, the next 2 "
                    "variables:\n"
                    "{\n"
                    "    \"guid\": \"the archive guid\",\n"
                    "    \"key\": \"the archive aes key\"\n"
                    "}");
            case EEndpointType::Mapping:
                return QStringLiteral(
                    "In order to make this work, you first need to understand JSON and its query language. If "
                    "you don't, please close this window. If your game does not use unversioned package "
                    "properties, please close this window. If you do understand what you are doing, you have to "
                    "know that the mapping expression supports up to 2 elements.\n"
                    "\n"
                    "    The first element is mandatory and will be assigned to the mapping download URL, which "
                    "can be all kinds of mapping but not Brotli compressed.\n"
                    "\n"
                    "    The second element is optional and will be assigned to the mapping file name. If "
                    "unspecified, said file name will be grabbed from the URL.");
        }
        // C#'s `_ => ""` arm.
        return QString();
    }

    EndpointEditor::EndpointEditor(EndpointSettings* endpoint, const QString& title, EEndpointType type,
                                   QWidget* parent)
        : QDialog(parent), _endpoint(endpoint), _type(type)
    {
        _isTested = _endpoint != nullptr && _endpoint->isValid();

        setWindowTitle(title);
        resize(760, 520);

        auto* layout = new QVBoxLayout(this);
        auto* grid = new QGridLayout;
        layout->addLayout(grid);

        grid->addWidget(new QLabel(QStringLiteral("URL")), 0, 0);
        _url = new QLineEdit(_endpoint != nullptr ? _endpoint->url() : QString());
        grid->addWidget(_url, 0, 1);
        auto* send = new QPushButton(QStringLiteral("Send"));
        grid->addWidget(send, 0, 2);

        grid->addWidget(new QLabel(QStringLiteral("Expression")), 1, 0);
        _path = new QLineEdit(_endpoint != nullptr ? _endpoint->path() : QString());
        grid->addWidget(_path, 1, 1);
        auto* test = new QPushButton(QStringLiteral("Test"));
        grid->addWidget(test, 1, 2);

        grid->addWidget(new QLabel(QStringLiteral("Overwrite")), 2, 0);
        _overwrite = new QCheckBox;
        _overwrite->setChecked(_endpoint != nullptr && _endpoint->overwrite());
        grid->addWidget(_overwrite, 2, 1);

        grid->addWidget(new QLabel(QStringLiteral("File Path")), 3, 0);
        _filePath = new QLineEdit(_endpoint != nullptr ? _endpoint->filePath() : QString());
        grid->addWidget(_filePath, 3, 1);

        auto* instruction = new QLabel(instructions(type));
        instruction->setObjectName(QStringLiteral("InstructionBox"));
        instruction->setWordWrap(true);
        layout->addWidget(instruction);

        auto* responses = new QGridLayout;
        _endpointResponse = new QPlainTextEdit;
        _endpointResponse->setObjectName(QStringLiteral("EndpointResponse"));
        _endpointResponse->setReadOnly(true);
        _endpointResponse->setFont(QFont(QStringLiteral("Consolas"), 10));
        _targetResponse = new QPlainTextEdit;
        _targetResponse->setObjectName(QStringLiteral("TargetResponse"));
        _targetResponse->setReadOnly(true);
        _targetResponse->setFont(QFont(QStringLiteral("Consolas"), 10));
        responses->addWidget(_endpointResponse, 0, 0);
        responses->addWidget(_targetResponse, 0, 1);
        layout->addLayout(responses, 1);

        auto* bottom = new QGridLayout;
        auto* label = new QLabel(_endpoint != nullptr ? _endpoint->label() : QString());
        bottom->addWidget(label, 0, 0);
        auto* ok = new QPushButton(QStringLiteral("OK"));
        ok->setDefault(true);
        auto* syntax = new QPushButton(QStringLiteral("Syntax"));
        auto* evaluator = new QPushButton(QStringLiteral("Evaluator"));
        bottom->addWidget(ok, 0, 1);
        bottom->addWidget(syntax, 0, 2);
        bottom->addWidget(evaluator, 0, 3);
        layout->addLayout(bottom);

        // C#'s two-way bindings. Url/Path also run OnTextChanged, which invalidates the endpoint on every
        // keystroke — hence the guard on IsLoaded upstream, reproduced here by connecting after the initial
        // text is in place.
        connect(_url, &QLineEdit::textChanged, this, [this](const QString& value)
        {
            if (_endpoint == nullptr) return;
            _endpoint->setUrl(value);
            _endpoint->setIsValid(false);
        });
        connect(_path, &QLineEdit::textChanged, this, [this](const QString& value)
        {
            if (_endpoint == nullptr) return;
            _endpoint->setPath(value);
            _endpoint->setIsValid(false);
        });
        connect(_filePath, &QLineEdit::textChanged, this, [this](const QString& value)
        {
            if (_endpoint != nullptr) _endpoint->setFilePath(value);
        });
        connect(_overwrite, &QCheckBox::toggled, this, [this](bool on)
        {
            if (_endpoint != nullptr) _endpoint->setOverwrite(on);
        });
        if (_endpoint != nullptr)
        {
            connect(_endpoint, &Framework::ViewModel::propertyChanged, label,
                    [this, label](const QString& propertyName)
                    {
                        if (propertyName == QStringLiteral("IsValid"))
                            label->setText(_endpoint->label());
                    });
        }

        connect(send, &QPushButton::clicked, this, &EndpointEditor::onSend);
        connect(test, &QPushButton::clicked, this, &EndpointEditor::onTest);
        connect(ok, &QPushButton::clicked, this, &EndpointEditor::onClick);
        connect(syntax, &QPushButton::clicked, this, &EndpointEditor::onSyntax);
        connect(evaluator, &QPushButton::clicked, this, &EndpointEditor::onEvaluator);
    }

    void EndpointEditor::onClick()
    {
        // C#: `DialogResult = _isTested && DataContext is EndpointSettings { IsValid: true }; Close();`
        if (_isTested && _endpoint != nullptr && _endpoint->isValid())
            accept();
        else
            reject();
    }

    void EndpointEditor::onSend()
    {
        // C#: ApiEndpointView.DynamicApi.GetRequestBody(default, endpoint.Url), rendered into EndpointResponse.
        emit deferred(QStringLiteral("Send"), QStringLiteral("ViewModels/ApiEndpointViewModel (DynamicApi)"));
    }

    void EndpointEditor::onTest()
    {
        // C#: endpoint.TryValidate(DynamicApi, _type, out var response); _isTested = true; and the serialized
        // response goes into TargetResponse. TryValidate is unported (see EndpointSettings.h), and _isTested
        // deliberately stays where the constructor left it — flipping it here without a real test would let
        // OnClick accept an endpoint nothing has checked.
        emit deferred(QStringLiteral("Test"), QStringLiteral("Settings/EndpointSettings (TryValidate)"));
    }

    void EndpointEditor::onSyntax()
    {
        ViewModels::Commands::MenuCommand::openUrl(QUrl(SyntaxLink));
    }

    void EndpointEditor::onEvaluator()
    {
        ViewModels::Commands::MenuCommand::openUrl(QUrl(EvaluatorLink));
    }
}

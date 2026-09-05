#ifndef RANDOMORDERINTFUNCTION_HPP
#define RANDOMORDERINTFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

#include <QDomDocument>

class RandomOrderIntFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit RandomOrderIntFunction(QWidget* parent = nullptr);

    void setOriginalXml(const QString& xml);
    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

    CMODFunction id() const override { return CMODFunction::functionRandomOrderInt; }
    QString xmlName() const override { return QStringLiteral("RandomOrderInt"); }
    QString displayName() const override { return QStringLiteral("RandomOrderInt"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
            FunctionReturnType::functionReturnPartialNum,
        };
    }

private:
    QDomDocument m_original;
    mutable QString m_poolId;
};

#endif // RANDOMORDERINTFUNCTION_HPP

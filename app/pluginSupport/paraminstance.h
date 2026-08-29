/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Copyright OpenFX and contributors to the OpenFX project.
#ifndef PARAM_INSTANCE_H
#define PARAM_INSTANCE_H

#include "olive/core/util/rational.h"
#include "pluginSupport/OlivePluginInstance.h"
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QVariant>
#include "ofxhParam.h"
#include "node/nodeundo.h"
#include "node/plugins/Plugin.h"
#include "core.h"
#include "undo/undocommand.h"
#include "common/Current.h"
#include <iostream>
#include <mutex>
#include <qlogging.h>
namespace olive
{
namespace plugin
{

inline bool
IsNormalisedCoordinateSystem(const OFX::Host::Param::Descriptor &descriptor)
{
	return descriptor.getDefaultCoordinateSystem() ==
		   kOfxParamCoordinatesNormalised;
}

inline void GetProjectExtent(double &xSize, double &ySize)
{
	auto &vp = Current::getInstance().currentVideoParams();
	xSize = vp.width() * vp.pixel_aspect_ratio().toDouble();
	ySize = vp.height();
}

inline double ToNormalised(double canonical, double extent)
{
	return extent > 0 ? canonical / extent : canonical;
}

inline double ToCanonical(double normalised, double extent)
{
	return extent > 0 ? normalised * extent : normalised;
}

inline QString ParamChangeLabel(const OFX::Host::Param::Descriptor &descriptor)
{
	return QStringLiteral("Change %1")
		.arg(QString::fromStdString(descriptor.getName()));
}
void SubmitUndoCommand(const std::shared_ptr<PluginNode> &node,
					   UndoCommand *command, const QString &label);

class NodeBoundParam {
public:
	virtual ~NodeBoundParam() = default;
	virtual void SetNode(const std::shared_ptr<PluginNode> &node) = 0;
};

class PushbuttonInstance : public OFX::Host::Param::PushbuttonInstance,
						   public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor *_descriptor;

public:
	PushbuttonInstance(std::shared_ptr<PluginNode> effect,
					   const std::string &name,
					   OFX::Host::Param::Descriptor &descriptor,
					   OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::PushbuttonInstance(descriptor, paramSet)
		, node(effect)
	{
		_descriptor = &descriptor;
	};
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
};

class IntegerInstance : public OFX::Host::Param::IntegerInstance,
						public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> _node;
	OFX::Host::Param::Descriptor &_descriptor;
	QString id;
	mutable std::mutex no_node_mutex_;
	bool has_value_ = false;
	int value_ = 0;

public:
	IntegerInstance(std::shared_ptr<PluginNode> node,
					OFX::Host::Param::Descriptor &descriptor,
					OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::IntegerInstance(descriptor, paramSet)
		, _node(node)
		, _descriptor(descriptor)
		, id(_descriptor.getName().c_str())
	{
		try {
			value_ = _descriptor.getProperties().getIntProperty(
				kOfxParamPropDefault);
			has_value_ = true;
		} catch (...) {
			value_ = 0;
			has_value_ = false;
		}
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		_node = new_node;
	}
	OfxStatus get(int &a)
	{
		if (!_node) {
			std::lock_guard<std::mutex> lock(no_node_mutex_);
			a = has_value_ ? value_ : 0;
			return kOfxStatOK;
		}
		if (id.isEmpty()) {
			return kOfxStatErrBadHandle;
		}
		QVariant variant = _node->GetStandardValue(id);

		if (variant.canConvert<int>()) {
			a = variant.toInt();
			return kOfxStatOK;
		}
		a = 0;
		return kOfxStatErrValue;
	}
	OfxStatus get(OfxTime time, int &data)
	{
		if (!_node) {
			std::lock_guard<std::mutex> lock(no_node_mutex_);
			data = has_value_ ? value_ : 0;
			return kOfxStatOK;
		}
		if (id.isEmpty()) {
			return kOfxStatErrBadHandle;
		}
		QVariant variant =
			_node->GetValueAtTime(id, rational::fromDouble(time));
		if (variant.canConvert<int>()) {
			data = variant.toInt();
			return kOfxStatOK;
		}
		data = 0;
		return kOfxStatErrValue;
	}
	OfxStatus set(int data)
	{
		if (!_node) {
			std::lock_guard<std::mutex> lock(no_node_mutex_);
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kInt, data);

		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(_node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(_node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, int data)
	{
		if (!_node) {
			std::lock_guard<std::mutex> lock(no_node_mutex_);
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		Node::SetValueAtTime(
			NodeInput(_node.get(), _descriptor.getName().c_str()),
			rational::fromDouble(time), data, 0, command, true);
		SubmitUndoCommand(_node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class DoubleInstance : public OFX::Host::Param::DoubleInstance,
					   public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	double value_ = 0.0;

public:
	DoubleInstance(std::shared_ptr<PluginNode> effect, const std::string &name,
				   OFX::Host::Param::Descriptor &descriptor,
				   OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::DoubleInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
		try {
			value_ = _descriptor.getProperties().getDoubleProperty(
				kOfxParamPropDefault);
			has_value_ = true;
		} catch (...) {
			value_ = 0.0;
			has_value_ = false;
		}
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(double &data)
	{
		if (!node) {
			data = has_value_ ? value_ : 0.0;
			return kOfxStatOK;
		}
		QVariant variant =
			node->GetStandardValue(_descriptor.getName().c_str());
		if (variant.canConvert<double>()) {
			data = variant.toDouble();
			if (IsNormalisedCoordinateSystem(_descriptor)) {
				double xSize, ySize;
				GetProjectExtent(xSize, ySize);
				data = ToNormalised(data, xSize);
			}
			return kOfxStatOK;
		}
		data = 0.0;
		return kOfxStatErrValue;
	}
	OfxStatus get(OfxTime time, double &data)
	{
		if (!node) {
			data = has_value_ ? value_ : 0.0;
			return kOfxStatOK;
		}
		QVariant variant = node->GetValueAtTime(_descriptor.getName().c_str(),
												rational::fromDouble(time));
		if (variant.canConvert<double>()) {
			data = variant.toDouble();
			if (IsNormalisedCoordinateSystem(_descriptor)) {
				double xSize, ySize;
				GetProjectExtent(xSize, ySize);
				data = ToNormalised(data, xSize);
			}
			return kOfxStatOK;
		}
		data = 0.0;
		return kOfxStatErrValue;
	}
	OfxStatus set(double data)
	{
		if (!node) {
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		double val = data;
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			val = ToCanonical(val, xSize);
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kFloat, val);
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, double data)
	{
		if (!node) {
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		double val = data;
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			val = ToCanonical(val, xSize);
		}
		auto command = new MultiUndoCommand();
		Node::SetValueAtTime(NodeInput(node.get(),
									   _descriptor.getName().c_str()),
							 rational::fromDouble(time), val, 0, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus derive(OfxTime, double &)
	{
		return kOfxStatErrUnsupported;
	}
	OfxStatus integrate(OfxTime, OfxTime, double &)
	{
		return kOfxStatErrUnsupported;
	}
};

class BooleanInstance : public OFX::Host::Param::BooleanInstance,
						public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	bool value_ = false;
	bool DefaultValue() const
	{
		return _descriptor.getProperties().getIntProperty(
				   kOfxParamPropDefault) != 0;
	}

public:
	BooleanInstance(std::shared_ptr<PluginNode> effect, const std::string &name,
					OFX::Host::Param::Descriptor &descriptor,
					OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::BooleanInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
		value_ = DefaultValue();
		has_value_ = true;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(bool &data)
	{
		if (!node) {
			data = has_value_ ? value_ : false;
			return kOfxStatOK;
		}
		QVariant variant =
			node->GetStandardValue(_descriptor.getName().c_str());
		if (variant.canConvert<bool>()) {
			data = variant.toBool();
			return kOfxStatOK;
		}
		data = DefaultValue();
		return kOfxStatOK;
	}
	OfxStatus get(OfxTime time, bool &data)
	{
		if (!node) {
			data = has_value_ ? value_ : false;
			return kOfxStatOK;
		}
		QVariant variant = node->GetValueAtTime(_descriptor.getName().c_str(),
												rational::fromDouble(time));
		if (variant.isNull()) {
			qWarning().noquote()
				<< "Boolean get failed: Varient is null" << time
				<< rational::fromDouble(time).toDouble();
		}
		if (!variant.isValid()) {
			qWarning().noquote()
				<< "Boolean get failed: Varient is invalid" << time
				<< rational::fromDouble(time).toDouble();
		}
		if (variant.canConvert<bool>()) {
			data = variant.toBool();
			return kOfxStatOK;
		}
		data = DefaultValue();
		return kOfxStatOK;
	}
	OfxStatus set(bool data)
	{
		if (!node) {
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kBoolean, data);
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, bool data)
	{
		if (!node) {
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		Node::SetValueAtTime(
			NodeInput(node.get(), _descriptor.getName().c_str()),
			rational::fromDouble(time), data, 0, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class ChoiceInstance : public OFX::Host::Param::ChoiceInstance,
					   public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	int value_ = 0;

public:
	ChoiceInstance(std::shared_ptr<PluginNode> effect, const std::string &name,
				   OFX::Host::Param::Descriptor &descriptor,
				   OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::ChoiceInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
		try {
			value_ = _descriptor.getProperties().getIntProperty(
				kOfxParamPropDefault);
			has_value_ = true;
		} catch (...) {
			value_ = 0;
			has_value_ = false;
		}
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(int &data)
	{
		if (!node) {
			data = has_value_ ? value_ : 0;
			return kOfxStatOK;
		}
		QVariant variant =
			node->GetStandardValue(_descriptor.getName().c_str());
		if (variant.canConvert<int>()) {
			data = variant.toInt();
			return kOfxStatOK;
		}
		data = 0;
		return kOfxStatErrValue;
	}
	OfxStatus get(OfxTime time, int &data)
	{
		if (!node) {
			data = has_value_ ? value_ : 0;
			return kOfxStatOK;
		}
		QVariant variant = node->GetValueAtTime(_descriptor.getName().c_str(),
												rational::fromDouble(time));
		if (variant.canConvert<int>()) {
			data = variant.toInt();
			return kOfxStatOK;
		}
		data = 0;
		return kOfxStatErrValue;
	}
	OfxStatus set(int data)
	{
		if (!node) {
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kCombo, data);
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, int data)
	{
		if (!node) {
			value_ = data;
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		Node::SetValueAtTime(
			NodeInput(node.get(), _descriptor.getName().c_str()),
			rational::fromDouble(time), data, 0, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class RGBAInstance : public OFX::Host::Param::RGBAInstance,
					 public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	double value_[4] = { 0.0, 0.0, 0.0, 0.0 };

public:
	RGBAInstance(std::shared_ptr<PluginNode> effect, const std::string &name,
				 OFX::Host::Param::Descriptor &descriptor,
				 OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::RGBAInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(double &r, double &g, double &b, double &a)
	{
		if (!node) {
			if (has_value_) {
				r = value_[0];
				g = value_[1];
				b = value_[2];
				a = value_[3];
			} else {
				r = g = b = a = 0.0;
			}
			return kOfxStatOK;
		}
		olive::core::Color c =
			node->GetStandardValue(_descriptor.getName().c_str())
				.value<olive::core::Color>();

		r = static_cast<double>(c.red());
		g = static_cast<double>(c.green());
		b = static_cast<double>(c.blue());
		a = static_cast<double>(c.alpha());
		return kOfxStatOK;
	}
	OfxStatus get(OfxTime time, double &r, double &g, double &b, double &a)
	{
		if (!node) {
			if (has_value_) {
				r = value_[0];
				g = value_[1];
				b = value_[2];
				a = value_[3];
			} else {
				r = g = b = a = 0.0;
			}
			return kOfxStatOK;
		}
		olive::core::Color c =
			node->GetValueAtTime(_descriptor.getName().c_str(),
								 rational::fromDouble(time))
				.value<olive::core::Color>();

		r = static_cast<double>(c.red());
		g = static_cast<double>(c.green());
		b = static_cast<double>(c.blue());
		a = static_cast<double>(c.alpha());
		return kOfxStatOK;
	}
	OfxStatus set(double r, double g, double b, double a)
	{
		if (!node) {
			value_[0] = r;
			value_[1] = g;
			value_[2] = b;
			value_[3] = a;
			has_value_ = true;
			return kOfxStatOK;
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kColor,
			QVariant::fromValue(olive::core::Color(r, g, b, a)));
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, double r, double g, double b, double a)
	{
		if (!node) {
			value_[0] = r;
			value_[1] = g;
			value_[2] = b;
			value_[3] = a;
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		const QString name = _descriptor.getName().c_str();
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), r, 0, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), g, 1, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), b, 2, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), a, 3, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class RGBInstance : public OFX::Host::Param::RGBInstance,
					public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	double value_[3] = { 0.0, 0.0, 0.0 };

public:
	RGBInstance(std::shared_ptr<PluginNode> effect, const std::string &name,
				OFX::Host::Param::Descriptor &descriptor,
				OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::RGBInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(double &r, double &g, double &b)
	{
		if (!node) {
			if (has_value_) {
				r = value_[0];
				g = value_[1];
				b = value_[2];
			} else {
				r = g = b = 0.0;
			}
			return kOfxStatOK;
		}
		olive::core::Color c =
			node->GetStandardValue(_descriptor.getName().c_str())
				.value<olive::core::Color>();

		r = static_cast<double>(c.red());
		g = static_cast<double>(c.green());
		b = static_cast<double>(c.blue());
		return kOfxStatOK;
	}
	OfxStatus get(OfxTime time, double &r, double &g, double &b)
	{
		if (!node) {
			if (has_value_) {
				r = value_[0];
				g = value_[1];
				b = value_[2];
			} else {
				r = g = b = 0.0;
			}
			return kOfxStatOK;
		}
		olive::core::Color c =
			node->GetValueAtTime(_descriptor.getName().c_str(),
								 rational::fromDouble(time))
				.value<olive::core::Color>();

		r = static_cast<double>(c.red());
		g = static_cast<double>(c.green());
		b = static_cast<double>(c.blue());
		return kOfxStatOK;
	}
	OfxStatus set(double r, double g, double b)
	{
		if (!node) {
			value_[0] = r;
			value_[1] = g;
			value_[2] = b;
			has_value_ = true;
			return kOfxStatOK;
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kColor,
			QVariant::fromValue(olive::core::Color(r, g, b)));
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, double r, double g, double b)
	{
		if (!node) {
			value_[0] = r;
			value_[1] = g;
			value_[2] = b;
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		const QString name = _descriptor.getName().c_str();
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), r, 0, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), g, 1, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), b, 2, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class Double2DInstance : public OFX::Host::Param::Double2DInstance,
						 public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	double value_[2] = { 0.0, 0.0 };

public:
	Double2DInstance(std::shared_ptr<PluginNode> effect,
					 const std::string &name,
					 OFX::Host::Param::Descriptor &descriptor,
					 OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::Double2DInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(double &x, double &y)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
			} else {
				x = y = 0.0;
			}
			return kOfxStatOK;
		}
		QVector2D vec = node->GetStandardValue(_descriptor.getName().c_str())
							.value<QVector2D>();
		x = static_cast<double>(vec.x());
		y = static_cast<double>(vec.y());
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			x = ToNormalised(x, xSize);
			y = ToNormalised(y, ySize);
		}
		return kOfxStatOK;
	}
	OfxStatus get(OfxTime time, double &x, double &y)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
			} else {
				x = y = 0.0;
			}
			return kOfxStatOK;
		}
		QVector2D vec = node->GetValueAtTime(_descriptor.getName().c_str(),
											 rational::fromDouble(time))
							.value<QVector2D>();
		x = static_cast<double>(vec.x());
		y = static_cast<double>(vec.y());
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			x = ToNormalised(x, xSize);
			y = ToNormalised(y, ySize);
		}
		return kOfxStatOK;
	}
	OfxStatus set(double x, double y)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			has_value_ = true;
			return kOfxStatOK;
		}
		double xv = x, yv = y;
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			xv = ToCanonical(xv, xSize);
			yv = ToCanonical(yv, ySize);
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kVec2, QVector2D(xv, yv));
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, double x, double y)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			has_value_ = true;
			return kOfxStatOK;
		}
		double xv = x, yv = y;
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			xv = ToCanonical(xv, xSize);
			yv = ToCanonical(yv, ySize);
		}
		auto command = new MultiUndoCommand();
		const QString name = _descriptor.getName().c_str();
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), xv, 0, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), yv, 1, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class Integer2DInstance : public OFX::Host::Param::Integer2DInstance,
						  public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	int value_[2] = { 0, 0 };

public:
	Integer2DInstance(std::shared_ptr<PluginNode> effect,
					  const std::string &name,
					  OFX::Host::Param::Descriptor &descriptor,
					  OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::Integer2DInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(int &x, int &y)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
			} else {
				x = y = 0;
			}
			return kOfxStatOK;
		}
		QVector2D vec = node->GetStandardValue(_descriptor.getName().c_str())
							.value<QVector2D>();
		x = static_cast<int>(vec.x());
		y = static_cast<int>(vec.y());
		return kOfxStatOK;
	}
	OfxStatus get(OfxTime time, int &x, int &y)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
			} else {
				x = y = 0;
			}
			return kOfxStatOK;
		}
		QVector2D vec = node->GetValueAtTime(_descriptor.getName().c_str(),
											 rational::fromDouble(time))
							.value<QVector2D>();
		x = static_cast<int>(vec.x());
		y = static_cast<int>(vec.y());
		return kOfxStatOK;
	}
	OfxStatus set(int x, int y)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			has_value_ = true;
			return kOfxStatOK;
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kVec2, QVector2D(x, y));
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, int x, int y)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		const QString name = _descriptor.getName().c_str();
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), x, 0, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), y, 1, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class Double3DInstance : public OFX::Host::Param::Double3DInstance,
						 public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	double value_[3] = { 0.0, 0.0, 0.0 };

public:
	Double3DInstance(std::shared_ptr<PluginNode> effect,
					 const std::string &name,
					 OFX::Host::Param::Descriptor &descriptor,
					 OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::Double3DInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(double &x, double &y, double &z)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
				z = value_[2];
			} else {
				x = y = z = 0.0;
			}
			return kOfxStatOK;
		}
		QVector3D vec = node->GetStandardValue(_descriptor.getName().c_str())
							.value<QVector3D>();
		x = static_cast<double>(vec.x());
		y = static_cast<double>(vec.y());
		z = static_cast<double>(vec.z());
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			x = ToNormalised(x, xSize);
			y = ToNormalised(y, ySize);
			z = ToNormalised(z, xSize);
		}
		return kOfxStatOK;
	}
	OfxStatus get(OfxTime time, double &x, double &y, double &z)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
				z = value_[2];
			} else {
				x = y = z = 0.0;
			}
			return kOfxStatOK;
		}
		QVector3D vec = node->GetValueAtTime(_descriptor.getName().c_str(),
											 rational::fromDouble(time))
							.value<QVector3D>();
		x = static_cast<double>(vec.x());
		y = static_cast<double>(vec.y());
		z = static_cast<double>(vec.z());
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			x = ToNormalised(x, xSize);
			y = ToNormalised(y, ySize);
			z = ToNormalised(z, xSize);
		}
		return kOfxStatOK;
	}
	OfxStatus set(double x, double y, double z)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			value_[2] = z;
			has_value_ = true;
			return kOfxStatOK;
		}
		double xv = x, yv = y, zv = z;
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			xv = ToCanonical(xv, xSize);
			yv = ToCanonical(yv, ySize);
			zv = ToCanonical(zv, xSize);
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kVec3, QVector3D(xv, yv, zv));
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, double x, double y, double z)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			value_[2] = z;
			has_value_ = true;
			return kOfxStatOK;
		}
		double xv = x, yv = y, zv = z;
		if (IsNormalisedCoordinateSystem(_descriptor)) {
			double xSize, ySize;
			GetProjectExtent(xSize, ySize);
			xv = ToCanonical(xv, xSize);
			yv = ToCanonical(yv, ySize);
			zv = ToCanonical(zv, xSize);
		}
		auto command = new MultiUndoCommand();
		const QString name = _descriptor.getName().c_str();
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), xv, 0, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), yv, 1, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), zv, 2, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class Integer3DInstance : public OFX::Host::Param::Integer3DInstance,
						  public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	int value_[3] = { 0, 0, 0 };

public:
	Integer3DInstance(std::shared_ptr<PluginNode> effect,
					  const std::string &name,
					  OFX::Host::Param::Descriptor &descriptor,
					  OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::Integer3DInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(int &x, int &y, int &z)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
				z = value_[2];
			} else {
				x = y = z = 0;
			}
			return kOfxStatOK;
		}
		QVector3D vec = node->GetStandardValue(_descriptor.getName().c_str())
							.value<QVector3D>();
		x = static_cast<int>(vec.x());
		y = static_cast<int>(vec.y());
		z = static_cast<int>(vec.z());
		return kOfxStatOK;
	}
	OfxStatus get(OfxTime time, int &x, int &y, int &z)
	{
		if (!node) {
			if (has_value_) {
				x = value_[0];
				y = value_[1];
				z = value_[2];
			} else {
				x = y = z = 0;
			}
			return kOfxStatOK;
		}
		QVector3D vec = node->GetValueAtTime(_descriptor.getName().c_str(),
											 rational::fromDouble(time))
							.value<QVector3D>();
		x = static_cast<int>(vec.x());
		y = static_cast<int>(vec.y());
		z = static_cast<int>(vec.z());
		return kOfxStatOK;
	}
	OfxStatus set(int x, int y, int z)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			value_[2] = z;
			has_value_ = true;
			return kOfxStatOK;
		}
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kVec3, QVector3D(x, y, z));
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, int x, int y, int z)
	{
		if (!node) {
			value_[0] = x;
			value_[1] = y;
			value_[2] = z;
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		const QString name = _descriptor.getName().c_str();
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), x, 0, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), y, 1, command, true);
		Node::SetValueAtTime(NodeInput(node.get(), name),
							 rational::fromDouble(time), z, 2, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class StringInstance : public OFX::Host::Param::StringInstance,
					   public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	std::string value_;

public:
	StringInstance(std::shared_ptr<PluginNode> effect, const std::string &name,
				   OFX::Host::Param::Descriptor &descriptor,
				   OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::StringInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
		try {
			value_ = _descriptor.getProperties().getStringProperty(
				kOfxParamPropDefault);
			has_value_ = true;
		} catch (...) {
			value_.clear();
			has_value_ = false;
		}
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(std::string &data)
	{
		if (!node) {
			data = has_value_ ? value_ : std::string();
			return kOfxStatOK;
		}
		QVariant variant =
			node->GetStandardValue(_descriptor.getName().c_str());
		if (variant.canConvert<QString>()) {
			data = variant.toString().toStdString();
			return kOfxStatOK;
		}
		data.clear();
		return kOfxStatErrValue;
	}
	OfxStatus get(OfxTime time, std::string &data)
	{
		if (!node) {
			data = has_value_ ? value_ : std::string();
			return kOfxStatOK;
		}
		QVariant variant = node->GetValueAtTime(_descriptor.getName().c_str(),
												rational::fromDouble(time));
		if (variant.canConvert<QString>()) {
			data = variant.toString().toStdString();
			return kOfxStatOK;
		}
		data.clear();
		return kOfxStatErrValue;
	}
	OfxStatus set(const char *data)
	{
		if (!node) {
			value_ = data ? data : "";
			has_value_ = true;
			return kOfxStatOK;
		}
		QString v = QString::fromUtf8(data);
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kText, v);
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, const char *data)
	{
		if (!node) {
			value_ = data ? data : "";
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		Node::SetValueAtTime(NodeInput(node.get(),
									   _descriptor.getName().c_str()),
							 rational::fromDouble(time),
							 QString::fromUtf8(data), 0, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class CustomInstance : public OFX::Host::Param::CustomInstance,
					   public NodeBoundParam {
protected:
	std::shared_ptr<PluginNode> node;
	OFX::Host::Param::Descriptor &_descriptor;
	bool has_value_ = false;
	std::string value_;

public:
	CustomInstance(std::shared_ptr<PluginNode> effect, const std::string &name,
				   OFX::Host::Param::Descriptor &descriptor,
				   OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::CustomInstance(descriptor, paramSet)
		, node(effect)
		, _descriptor(descriptor)
	{
		(void)name;
	}
	void SetNode(const std::shared_ptr<PluginNode> &new_node) override
	{
		node = new_node;
	}
	OfxStatus get(std::string &data)
	{
		if (!node) {
			data = has_value_ ? value_ : std::string();
			return kOfxStatOK;
		}
		QVariant variant =
			node->GetStandardValue(_descriptor.getName().c_str());
		if (variant.canConvert<QByteArray>()) {
			data = variant.toByteArray().toStdString();
			return kOfxStatOK;
		}
		if (variant.canConvert<QString>()) {
			data = variant.toString().toStdString();
			return kOfxStatOK;
		}
		data.clear();
		return kOfxStatErrValue;
	}
	OfxStatus get(OfxTime time, std::string &data)
	{
		if (!node) {
			data = has_value_ ? value_ : std::string();
			return kOfxStatOK;
		}
		QVariant variant = node->GetValueAtTime(_descriptor.getName().c_str(),
												rational::fromDouble(time));
		if (variant.canConvert<QByteArray>()) {
			data = variant.toByteArray().toStdString();
			return kOfxStatOK;
		}
		if (variant.canConvert<QString>()) {
			data = variant.toString().toStdString();
			return kOfxStatOK;
		}
		data.clear();
		return kOfxStatErrValue;
	}
	OfxStatus set(const char *data)
	{
		if (!node) {
			value_ = data ? data : "";
			has_value_ = true;
			return kOfxStatOK;
		}
		QByteArray v = QByteArray(data);
		SplitValue split = NodeValue::split_normal_value_into_track_values(
			NodeValue::kBinary, v);
		auto command = new NodeParamSetSplitStandardValueCommand(
			NodeInput(node.get(), _descriptor.getName().c_str()), split);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
	OfxStatus set(OfxTime time, const char *data)
	{
		if (!node) {
			value_ = data ? data : "";
			has_value_ = true;
			return kOfxStatOK;
		}
		auto command = new MultiUndoCommand();
		Node::SetValueAtTime(
			NodeInput(node.get(), _descriptor.getName().c_str()),
			rational::fromDouble(time), QByteArray(data), 0, command, true);
		SubmitUndoCommand(node, command, ParamChangeLabel(_descriptor));
		return kOfxStatOK;
	}
};

class GroupInstance : public OFX::Host::Param::GroupInstance {
public:
	GroupInstance(OFX::Host::Param::Descriptor &descriptor,
				  OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::GroupInstance(descriptor, paramSet)
	{
	}
};

class PageInstance : public OFX::Host::Param::PageInstance {
public:
	PageInstance(OFX::Host::Param::Descriptor &descriptor,
				 OFX::Host::Param::SetInstance *paramSet = nullptr)
		: OFX::Host::Param::PageInstance(descriptor, paramSet)
	{
	}
};
}
}

#endif // HOST_DEMO_PARAM_INSTANCE_H

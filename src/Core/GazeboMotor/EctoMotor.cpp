//
// Created by hiram on 9/08/19.
//

#include <stdexcept>
#include <cmath>
#include <chrono>
#include "EctoMotor.h"


EctoMotor::EctoMotor(int motorID, const std::string &motorName) {
	this->id = motorID;
	this->name = motorName;
	
}

void EctoMotor::set(double value) {
	this->set(value, this->controlMode);
}

void EctoMotor::set(double value, MotorControlMode newControlMode) {
	
	if(controlMode != newControlMode){
		setControlMode(controlMode);
	}
	
	lastSetpoint = std::make_pair(newControlMode, value);
	
	switch (this->controlMode) {
		case MotorControlMode::Percent:
			this->setOutputPercent(value);
			break;
		case MotorControlMode::Velocity:
			this->setVelocitySetpoint(value);
			break;
		case MotorControlMode::Position:
			this->setPositionSetpoint(value);
			break;
		case MotorControlMode::MotionMagic:
			break;
		case MotorControlMode::Current:
			this->setMotorOutputByCurrent(value);
			break;
		case MotorControlMode::Voltage:
			this->setVoltageOutput(value);
			break;
		default:
			throw std::logic_error("Invalid MotorControlMode");
	}
}

std::string EctoMotor::getName() const {
	return name;
}

void EctoMotor::setControlMode(MotorControlMode controlMode) {
	this->controlMode = controlMode;
}

MotorControlMode EctoMotor::getControlMode() const {
	return this->controlMode;
}

void EctoMotor::setFeedbackMode(MotorFeedbackMode feedbackMode) {
	this->feedbackMode = feedbackMode;
	switch (this->feedbackMode) {
		case MotorFeedbackMode::None:
			throw std::runtime_error("Cant set feedbackmode to None in motor with name: " + name);
		case MotorFeedbackMode::QuadEncoder:
			this->setQuadAsClosedLoopSource();
			break;
		case MotorFeedbackMode::Potentiometer:
			this->setPotAsClosedLoopSource();
			break;
	}
}

MotorFeedbackMode EctoMotor::getFeedbackMode() const {
	return this->feedbackMode;
}

int EctoMotor::getId() const {
	return id;
}

bool EctoMotor::isDisabled() const {
	return disabled;
}

double EctoMotor::getPosition() const {
	switch (feedbackMode) {
		case MotorFeedbackMode::None:
			return 0;
		case MotorFeedbackMode::QuadEncoder:
			return getQuadPosition();
		case MotorFeedbackMode::Potentiometer:
			return getPotPosition();
		default:
			return 0;
	}
}

double EctoMotor::getVelocity() const {
	switch (feedbackMode) {
		case MotorFeedbackMode::None:
			return 0;
		case MotorFeedbackMode::QuadEncoder:
			return getQuadVelocity();
		case MotorFeedbackMode::Potentiometer:
			return getPotVelocity();
		default:
			return 0;
	}
}

std::pair<MotorControlMode, double> EctoMotor::getLastSetpoint() const {
	return lastSetpoint;
}

void EctoMotor::outputSet(double set) {
	this->set(set, MotorControlMode::Voltage);
}
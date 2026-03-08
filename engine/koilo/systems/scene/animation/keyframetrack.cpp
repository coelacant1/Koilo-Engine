#include <ptx/systems/scene/animation/keyframetrack.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>

namespace {
constexpr float kSecondsPerMillisecond = 1.0f / 1000.0f;

float CurrentSeconds() {
	return static_cast<float>(ptx::Time::Millis()) * kSecondsPerMillisecond;
}
} // namespace

KeyFrameTrack::KeyFrameTrack(float min,
							 float max,
							 InterpolationMethod interpMethod,
							 std::size_t parameterCapacity,
							 std::size_t keyFrameCapacity)
	: min_(min)
	, max_(max)
	, parameterValue_(ClampValue(min))
	, currentTime_(0.0f)
	, lastUpdateSeconds_(CurrentSeconds())
	, isActive_(true)
	, playbackSpeed_(1.0f)
	, interpMethod_(interpMethod)
	, parameterCapacity_(parameterCapacity)
	, keyFrameCapacity_(keyFrameCapacity) {
	parameters_.reserve(parameterCapacity_);
	keyFrames_.reserve(keyFrameCapacity_);
	UpdateFrameRange();
}

float KeyFrameTrack::GetCurrentTime() const noexcept {
	return currentTime_;
}

void KeyFrameTrack::SetCurrentTime(float setTime) {
	currentTime_ = setTime;
	if (!keyFrames_.empty() && stopFrameTime_ >= startFrameTime_) {
		const float duration = stopFrameTime_ - startFrameTime_;
		if (duration > Mathematics::EPSILON) {
			while (currentTime_ > stopFrameTime_) {
				currentTime_ -= duration;
			}
			while (currentTime_ < startFrameTime_) {
				currentTime_ += duration;
			}
		} else {
			currentTime_ = startFrameTime_;
		}
	}
	lastUpdateSeconds_ = CurrentSeconds();
}

void KeyFrameTrack::Pause() {
	isActive_ = false;
}

void KeyFrameTrack::Play() {
	isActive_ = true;
	lastUpdateSeconds_ = CurrentSeconds();
}

void KeyFrameTrack::SetPlaybackSpeed(float playbackSpeed) noexcept {
	playbackSpeed_ = playbackSpeed;
}

void KeyFrameTrack::SetMin(float min) noexcept {
	min_ = min;
	if (min_ > max_) {
		max_ = min_;
	}
	parameterValue_ = ClampValue(parameterValue_);
	for (KeyFrame& frame : keyFrames_) {
		frame.Value = ClampValue(frame.Value);
	}
	for (float* parameter : parameters_) {
		if (parameter) {
			*parameter = parameterValue_;
		}
	}
}

void KeyFrameTrack::SetMax(float max) noexcept {
	max_ = max;
	if (max_ < min_) {
		min_ = max_;
	}
	parameterValue_ = ClampValue(parameterValue_);
	for (KeyFrame& frame : keyFrames_) {
		frame.Value = ClampValue(frame.Value);
	}
	for (float* parameter : parameters_) {
		if (parameter) {
			*parameter = parameterValue_;
		}
	}
}

void KeyFrameTrack::SetRange(float min, float max) noexcept {
	if (min <= max) {
		min_ = min;
		max_ = max;
	} else {
		min_ = max;
		max_ = min;
	}
	parameterValue_ = ClampValue(parameterValue_);
	for (KeyFrame& frame : keyFrames_) {
		frame.Value = ClampValue(frame.Value);
	}
	for (float* parameter : parameters_) {
		if (parameter) {
			*parameter = parameterValue_;
		}
	}
}

void KeyFrameTrack::AddParameter(float* parameter) {
	if (!parameter) {
		return;
	}
	if (parameters_.size() >= parameterCapacity_) {
		return;
	}
	if (std::find(parameters_.begin(), parameters_.end(), parameter) != parameters_.end()) {
		return;
	}
	parameters_.push_back(parameter);
	*parameter = parameterValue_;
}

void KeyFrameTrack::AddKeyFrame(float time, float value) {
	AddKeyFrame(KeyFrame(time, value));
}

void KeyFrameTrack::AddKeyFrame(const KeyFrame& keyFrame) {
	if (keyFrames_.size() >= keyFrameCapacity_) {
		return;
	}

	KeyFrame frame = keyFrame;
	frame.Value = ClampValue(frame.Value);
	InsertKeyFrame(frame);
	UpdateFrameRange();

	if (keyFrames_.size() == 1) {
		currentTime_ = frame.Time;
		parameterValue_ = frame.Value;
	}
}

void KeyFrameTrack::RemoveKeyFrame(std::size_t index) {
	if (index >= keyFrames_.size()) {
		return;
	}
	keyFrames_.erase(keyFrames_.begin() + static_cast<std::ptrdiff_t>(index));
	UpdateFrameRange();

	if (!keyFrames_.empty()) {
		currentTime_ = Mathematics::Constrain(currentTime_, startFrameTime_, stopFrameTime_);
	} else {
		currentTime_ = 0.0f;
		parameterValue_ = ClampValue(parameterValue_);
	}
}

float KeyFrameTrack::GetParameterValue() const noexcept {
	return parameterValue_;
}

void KeyFrameTrack::Reset() {
	parameterValue_ = ClampValue(min_);
	currentTime_ = !keyFrames_.empty() ? keyFrames_.front().Time : 0.0f;
	lastUpdateSeconds_ = CurrentSeconds();

	for (float* parameter : parameters_) {
		if (parameter) {
			*parameter = parameterValue_;
		}
	}
}

float KeyFrameTrack::Update() {
	const float currentSeconds = CurrentSeconds();

	if (!isActive_ || keyFrames_.empty()) {
		lastUpdateSeconds_ = currentSeconds;
		for (float* parameter : parameters_) {
			if (parameter) {
				*parameter = parameterValue_;
			}
		}
		return parameterValue_;
	}

	if (keyFrames_.size() > 1 && stopFrameTime_ > startFrameTime_) {
		const float duration = stopFrameTime_ - startFrameTime_;
		if (duration > Mathematics::EPSILON) {
			const float deltaSeconds = (currentSeconds - lastUpdateSeconds_) * playbackSpeed_;
			currentTime_ += deltaSeconds;

			while (currentTime_ > stopFrameTime_) {
				currentTime_ -= duration;
			}
			while (currentTime_ < startFrameTime_) {
				currentTime_ += duration;
			}
		} else {
			currentTime_ = startFrameTime_;
		}
	} else {
		currentTime_ = keyFrames_.front().Time;
	}

	float parameter = keyFrames_.front().Value;

	if (keyFrames_.size() == 1) {
		parameter = keyFrames_.front().Value;
	} else {
		auto upper = std::upper_bound(
			keyFrames_.begin(),
			keyFrames_.end(),
			currentTime_,
			[](float value, const KeyFrame& frame) {
				return value < frame.Time;
			});

		std::size_t nextIndex = 0;
		if (upper == keyFrames_.end()) {
			nextIndex = keyFrames_.size() - 1;
		} else {
			nextIndex = static_cast<std::size_t>(std::distance(keyFrames_.begin(), upper));
		}

		std::size_t prevIndex = nextIndex == 0 ? 0 : nextIndex - 1;
		if (upper == keyFrames_.begin()) {
			prevIndex = 0;
			nextIndex = keyFrames_.size() > 1 ? 1 : 0;
		}

		const KeyFrame& prevFrame = keyFrames_[prevIndex];
		const KeyFrame& nextFrame = keyFrames_[std::min(nextIndex, keyFrames_.size() - 1)];

		if (&prevFrame == &nextFrame || std::fabs(nextFrame.Time - prevFrame.Time) <= Mathematics::EPSILON) {
			parameter = prevFrame.Value;
		} else {
			float ratio = (currentTime_ - prevFrame.Time) / (nextFrame.Time - prevFrame.Time);
			ratio = Mathematics::Constrain(ratio, 0.0f, 1.0f);

			switch (interpMethod_) {
				case Cosine:
					parameter = Mathematics::CosineInterpolation(prevFrame.Value, nextFrame.Value, ratio);
					break;
				case Step:
					parameter = prevFrame.Value;
					break;
				case Linear:
				default:
					parameter = Mathematics::Map(ratio, 0.0f, 1.0f, prevFrame.Value, nextFrame.Value);
					break;
			}
		}
	}

	parameterValue_ = parameter;
	lastUpdateSeconds_ = currentSeconds;

	for (float* paramPtr : parameters_) {
		if (paramPtr) {
			*paramPtr = parameterValue_;
		}
	}

	return parameterValue_;
}

float KeyFrameTrack::ClampValue(float value) const {
	return Mathematics::Constrain(value, min_, max_);
}

void KeyFrameTrack::InsertKeyFrame(const KeyFrame& keyFrame) {
	const auto insertPos = std::lower_bound(
		keyFrames_.begin(),
		keyFrames_.end(),
		keyFrame.Time,
		[](const KeyFrame& lhs, float rhs) {
			return lhs.Time < rhs;
		});

	if (insertPos == keyFrames_.end()) {
		keyFrames_.push_back(keyFrame);
		return;
	}

	if (Mathematics::IsClose(insertPos->Time, keyFrame.Time, Mathematics::EPSILON)) {
		*insertPos = keyFrame;
		return;
	}

	keyFrames_.insert(insertPos, keyFrame);
}

void KeyFrameTrack::UpdateFrameRange() {
	if (keyFrames_.empty()) {
		startFrameTime_ = Mathematics::FLTMAX;
		stopFrameTime_ = Mathematics::FLTMIN;
		return;
	}

	startFrameTime_ = keyFrames_.front().Time;
	stopFrameTime_ = keyFrames_.back().Time;
	currentTime_ = Mathematics::Constrain(currentTime_, startFrameTime_, stopFrameTime_);
}


//
// Created by brunocooper on 5/5/23.
//

#include "cppgpio/MultiplexerPushButton8.hpp"


namespace GPIO
{
	MultiplexerControl8::MultiplexerControl8(
		uint32_t InPinA, uint32_t InPinB, uint32_t InPinC, std::chrono::nanoseconds InSleepMultiplexerAction)
		: MultiplexerPinA{ InPinA }
		, MultiplexerPinB{ InPinB }
		, MultiplexerPinC{ InPinC }
		, SleepMultiplexerAction{ InSleepMultiplexerAction }
	{
	}


	MultiplexerControl8::~MultiplexerControl8()
	{
		stop();
	}


	bool MultiplexerControl8::start()
	{
		if (thread_loop != nullptr)
		{
			return false;
		}

		thread_loop = std::make_unique<std::thread>(&MultiplexerControl8::event_loop, this);

		return true;
	}


	bool MultiplexerControl8::stop()
	{
		if (thread_loop == nullptr)
		{
			return false;
		}

		bTerminate = true;

		thread_loop->join();
		thread_loop.reset(nullptr);

		return true;
	}


	void MultiplexerControl8::event_loop()
	{
		UnionMultiplexer8 MultiplexerConvert{ 0 };

		while (!bTerminate)
		{
			for (std::size_t idx{ 0 }; idx < 8 && !bTerminate; ++idx)
			{
				MultiplexerConvert.Index = idx;
				MultiplexerConvert.Bool.bPinA ? MultiplexerPinA.on() : MultiplexerPinA.off();
				MultiplexerConvert.Bool.bPinB ? MultiplexerPinB.on() : MultiplexerPinB.off();
				MultiplexerConvert.Bool.bPinC ? MultiplexerPinC.on() : MultiplexerPinC.off();

				for (auto& notify : change_notify)
				{
					if (notify)
					{
						notify(MultiplexerConvert);
					}
				}

				/** Wait until checking next value (to avoid cpu saturation) */
				std::this_thread::sleep_for(SleepMultiplexerAction);
			}

			auto result = std::remove_if(change_notify.begin(), change_notify.end(),
				[](const std::function<void(const UnionMultiplexer8&)>& notify)
				{
					return !notify;
				});
		}
	}


	MultiplexerPushButton8::MultiplexerPushButton8(uint32_t InPinOutput, MultiplexerControl8& InMultiplexerControl,
		std::chrono::nanoseconds InMinTriggerAction, std::chrono::nanoseconds InMinHoldAction)
		: ButtonIn{ InPinOutput, GPIO::GPIO_PULL::UP }
		, MultiplexerControl{ InMultiplexerControl }
		, MinTriggerAction{ InMinTriggerAction }
		, MinHeldAction{ InMinHoldAction }
	{
	}


	MultiplexerPushButton8::~MultiplexerPushButton8()
	{
		stop();
	}


	bool MultiplexerPushButton8::start()
	{
		ButtonIn.start();
		MultiplexerControl.change_notify.emplace_back(
			[this](auto&& PH1)
			{
				OnMultiplexerChange(std::forward<decltype(PH1)>(PH1));
			});

		return true;
	}


	bool MultiplexerPushButton8::stop()
	{
		ButtonIn.stop();
		return true;
	}


	void MultiplexerPushButton8::OnMultiplexerChange(const GPIO::UnionMultiplexer8& CurrentIndex)
	{
		const bool InputRead{ ButtonIn.is_on() };
		const auto TimeNow{ std::chrono::steady_clock::now() };

		/** Reset the time in every unpressed button */
		for (std::size_t internal_idx{ 0 }; internal_idx < ButtonLastPushed.size(); ++internal_idx)
		{
			if (internal_idx == CurrentIndex.Index)
			{
				continue;
			}

			if (!ButtonLastState[internal_idx])
			{
				ButtonLastPushed[internal_idx] = TimeNow;
			}
		}

		/**
		 * Button was released, check the press valid (was it held? or at least was hold long enough for a tap)
		 */
		if (!InputRead)
		{
			const auto DeltaTime{ TimeNow - ButtonLastPushed[CurrentIndex.Index] };
			if (ButtonCheckHeldState[CurrentIndex.Index] && DeltaTime >= MinTriggerAction)
			{
				ButtonLastPushed[CurrentIndex.Index] = TimeNow;

				if (f_pushed)
				{
					f_pushed(CurrentIndex.Index, DeltaTime);
				}
			}
		}

		/** Update state if it's different than last state */
		if (ButtonLastState[CurrentIndex.Index] != InputRead)
		{
			ButtonLastPushed[CurrentIndex.Index] = TimeNow;
			ButtonLastState[CurrentIndex.Index] = InputRead;
			ButtonCheckHeldState[CurrentIndex.Index] = InputRead;
		}

		/** Was it held long enough to consider it a hold action? */
		if (ButtonCheckHeldState[CurrentIndex.Index])
		{
			const auto TimeHeld{ TimeNow - ButtonLastPushed[CurrentIndex.Index] };
			if (TimeHeld >= MinHeldAction)
			{
				ButtonCheckHeldState[CurrentIndex.Index] = false;
				ButtonLastPushed[CurrentIndex.Index] = TimeNow;

				if (f_held)
				{
					f_held(CurrentIndex.Index, TimeHeld);
				}
			}
		}
	}
} // namespace GPIO

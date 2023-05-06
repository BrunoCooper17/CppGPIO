//
// Created by brunocooper on 5/5/23.
//

#include "cppgpio/MultiplexerPushButton8.hpp"


namespace GPIO
{

	MultiplexerPushButton8::MultiplexerPushButton8(uint32_t InPinOutput, uint32_t InPinA, uint32_t InPinB,
												   uint32_t InPinC, std::chrono::nanoseconds InSleepMultiplexerAction, std::chrono::nanoseconds InMinTriggerAction,
												   std::chrono::nanoseconds InMinHoldAction)
		: ButtonIn{ InPinOutput, GPIO::GPIO_PULL::UP }
		, MultiplexerPinA{ InPinA }
		, MultiplexerPinB{ InPinB }
		, MultiplexerPinC{ InPinC }
		, SleepMultiplexerAction{ InSleepMultiplexerAction }
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
		if (thread_loop != nullptr)
		{
			return false;
		}

		ButtonIn.start();
		thread_loop = std::make_unique<std::thread>(&MultiplexerPushButton8::event_loop, this);

		return true;
	}


	bool MultiplexerPushButton8::stop()
	{
		if (thread_loop == nullptr)
		{
			return false;
		}

		ButtonIn.stop();
		bTerminate = true;

		thread_loop->join();
		thread_loop.reset(nullptr);

		return true;
	}


	void MultiplexerPushButton8::event_loop()
	{
		UnionMultiplexer8 MultiplexerConvert{ 0 };

		while (!bTerminate)
		{
			for (std::size_t idx{ 0 }; idx < ButtonLastPushed.size() && !bTerminate; ++idx)
			{
				MultiplexerConvert.Index = idx;
				MultiplexerConvert.Bool.bPinA ? MultiplexerPinA.on() : MultiplexerPinA.off();
				MultiplexerConvert.Bool.bPinB ? MultiplexerPinB.on() : MultiplexerPinB.off();
				MultiplexerConvert.Bool.bPinC ? MultiplexerPinC.on() : MultiplexerPinC.off();

				const bool InputRead{ ButtonIn.is_on() };
				const auto TimeNow{ std::chrono::steady_clock::now() };

				/** Reset the time in every unpressed button */
				for (std::size_t internal_idx{ 0 }; internal_idx < ButtonLastPushed.size(); ++internal_idx)
				{
					if (internal_idx == idx)
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
					const auto DeltaTime{ TimeNow - ButtonLastPushed[idx] };
					if (ButtonCheckHeldState[idx] && DeltaTime >= MinTriggerAction)
					{
						ButtonLastPushed[idx] = TimeNow;

						if (f_pushed)
						{
							f_pushed(idx, DeltaTime);
						}
					}
				}

				/** Update state if it's different than last state */
				if (ButtonLastState[idx] != InputRead)
				{
					ButtonLastPushed[idx] = TimeNow;
					ButtonLastState[idx] = InputRead;
					ButtonCheckHeldState[idx] = InputRead;
				}

				/** Was it held long enough to consider it a hold action? */
				if (ButtonCheckHeldState[idx])
				{
					const auto TimeHeld{ TimeNow - ButtonLastPushed[idx] };
					if (TimeHeld >= MinHeldAction)
					{
						ButtonCheckHeldState[idx] = false;
						ButtonLastPushed[idx] = TimeNow;

						if (f_held)
						{
							f_held(idx, TimeHeld);
						}
					}
				}

				/** Wait until checking next value (to avoid cpu saturation) */
				std::this_thread::sleep_for(SleepMultiplexerAction);
			}
		}
	}

} // namespace GPIO

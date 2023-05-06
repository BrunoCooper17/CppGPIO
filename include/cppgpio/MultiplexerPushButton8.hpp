//
// Created by brunocooper on 5/5/23.
//

#ifndef CPPGPIO_MULTIPLEXERPUSHBUTTON8_H
#define CPPGPIO_MULTIPLEXERPUSHBUTTON8_H


#include <cppgpio.hpp>


namespace GPIO
{
	struct FMultiplexerBool8
	{
		uint8_t bPinC : 1;
		uint8_t bPinB : 1;
		uint8_t bPinA : 1;
	};


	union UnionMultiplexer8
	{
		uint8_t Index;
		FMultiplexerBool8 Bool;

		explicit UnionMultiplexer8(uint8_t IndexValue)
			: Index{ IndexValue }
		{
		}
	};


	/**
	 * #todo: Add description
	 */
	class MultiplexerBase
	{
	public:
		virtual void OnMultiplexerChange(const UnionMultiplexer8& CurrentIndex) = 0;
	};


	/**
	 * #todo: Add description
	 */
	class MultiplexerControl8
	{
	public:
		MultiplexerControl8(
			/** Multiplexer pin A */
			uint32_t InPinA,
			/** Multiplexer pin B */
			uint32_t InPinB,
			/** Multiplexer pin C */
			uint32_t InPinC,
			/** Multiplexer check interval */
			std::chrono::nanoseconds InSleepMultiplexerAction = std::chrono::microseconds(125));

		virtual ~MultiplexerControl8();

		std::vector<std::function<void(const UnionMultiplexer8&)>> change_notify;

		/**
		 * From InputDetect::start (buttons.hpp)
		 * After completion of class initialization (that is, registration of callables
		 * at the various function variables), signal here that the input detection
		 * thread could finally be run. If start() is never called, no detection will
		 * ever happen.
		 */
		bool start();

		/**
		 * From InputDetect::stop (buttons.hpp)
		 * it is also possible to stop the running thread, but you would normally not
		 * need to do it - the destructor does it automatically. If you call stop(), then
		 * be prepared that it could take up to 500ms for it to return (or even longer if
		 * the thread is currently not waiting for an event, but executing some long
		 * running custom task in your code).
		 */
		bool stop();


	private:
		GPIO::DigitalOut MultiplexerPinA;
		GPIO::DigitalOut MultiplexerPinB;
		GPIO::DigitalOut MultiplexerPinC;

		std::chrono::nanoseconds SleepMultiplexerAction{ std::chrono::microseconds(125) };

		bool bTerminate{ false };
		std::unique_ptr<std::thread> thread_loop{ nullptr };

		void event_loop();
	};


	/**
	 * #todo: Add description
	 * #todo: Refactor multiplexer out values so we can reuse it (and send an event to notify about new value to read)
	 */
	class MultiplexerPushButton8 : public MultiplexerBase
	{
	public:
		MultiplexerPushButton8(
			/** Multiplexer Output pin */
			uint32_t InPinInput,
			/** Multiplexer control */
			MultiplexerControl8& MultiplexerControl,
			/** Define interval between two events */
			std::chrono::nanoseconds InMinTriggerAction = std::chrono::milliseconds(2),
			/** Define minimum amount of time the hast to be triggered to consider it Hold action */
			std::chrono::nanoseconds InMinHoldAction = std::chrono::milliseconds(750));

		virtual ~MultiplexerPushButton8();

		/**
		 * Gets called when a button is pushed.
		 * Register with a lambda expression or std::bind()
		 */
		std::function<void(uint8_t, const std::chrono::nanoseconds)> f_pushed{ nullptr };

		/**
		 * Gets called when a button is held.
		 * Register with a lambda expression or std::bind()
		 */
		std::function<void(uint8_t, const std::chrono::nanoseconds)> f_held{ nullptr };

		/**
		 * From InputDetect::start (buttons.hpp)
		 * After completion of class initialization (that is, registration of callables
		 * at the various function variables), signal here that the input detection
		 * thread could finally be run. If start() is never called, no detection will
		 * ever happen.
		 */
		bool start();

		/**
		 * From InputDetect::stop (buttons.hpp)
		 * it is also possible to stop the running thread, but you would normally not
		 * need to do it - the destructor does it automatically. If you call stop(), then
		 * be prepared that it could take up to 500ms for it to return (or even longer if
		 * the thread is currently not waiting for an event, but executing some long
		 * running custom task in your code).
		 */
		bool stop();


	private:
		GPIO::DirectIn ButtonIn;

		GPIO::MultiplexerControl8& MultiplexerControl;

		std::chrono::nanoseconds MinTriggerAction{ std::chrono::milliseconds(3) };
		std::chrono::nanoseconds MinHeldAction{ std::chrono::milliseconds(750) };

		std::array<std::chrono::steady_clock::time_point, 8> ButtonLastPushed;
		std::array<bool, 8> ButtonLastState{ false };
		std::array<bool, 8> ButtonCheckHeldState{ false };

		void OnMultiplexerChange(const UnionMultiplexer8& CurrentIndex) override;
	};

}; // namespace GPIO

#endif // CPPGPIO_MULTIPLEXERPUSHBUTTON8_H

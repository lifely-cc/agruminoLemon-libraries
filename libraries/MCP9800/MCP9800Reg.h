// MCP Registers
#define REG_TEMP			0x00	// Temperatur register
#define REG_CONFIG			0x01	// Configuration register
#define REG_HYSTERISIS		0x02	// Temperature Hysteresis register
#define REG_LIMIT			0x03	// Temperature Limit-set register

// Config bits
#define CONFIG_ONE_SHOT		0x07	// One shot enabled/disabled. Disabled by default
#define CONFIG_ADC_RES		0x05	// ADC resolution: 00 = 9bit (0.5c), 01 = 10bit (0.25c), 10 = 11bit (0.125c), 11 = 12bit (0.0625c)
#define CONFIG_FAULT_QUEUE	0x03	// Fault queue bits, 00 = 1 (default), 01 = 2, 10 = 4, 11 = 6
#define CONFIG_ALERT_POL	0x04	// Alert polarity (high/low). Default low
#define CONFIG_COMP_INT		0x03	// 1 = Interrupt mode, 0 = Comparator mode (default)
#define CONFIG_SHUTDOWN		0x02	// 1 = Enable shutdown, 0 = Disable shutdown (default)
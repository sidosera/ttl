import { Box, useApp, Text } from 'ink';
import TextInput from 'ink-text-input';
import React, { useState } from 'react';

export type ReplProps = Record<string, never>;

export const Repl: React.FC<ReplProps> = () => {
	const { exit } = useApp();
	const [input, setInput] = useState('');
	const [message, setMessage] = useState<string | null>(null);

	const handleSubmit = (value: string) => {
		const trimmed = value.trim();

		if (!trimmed) {
			return;
		}

		if (trimmed === '/q') {
			setMessage('Bye.');
			exit();
			return;
		}

		setInput('');

		if (trimmed.length != 0) {
			setMessage(`Unknown command: ${trimmed}`);
		}

	};

	return (
		<Box flexDirection="column" padding={1}>
			<Box>
				<Text color="gray">&gt; </Text>
				<TextInput value={input} onChange={setInput} onSubmit={handleSubmit} />
			</Box>
			{message && (
				<Box marginBottom={1}>
					<Text color="yellow">{message}</Text>
				</Box>
			)}
		</Box>
	);
};

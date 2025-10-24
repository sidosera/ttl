import { Box, useApp, Text } from 'ink';
import React, { useState } from 'react';
import { useRuntime } from '@/context/RuntimeContext.js';
import { HistoryInput } from '@/HistoryInput.js';

export type ReplProps = Record<string, never>;

const detectSchema = (query: string): string | null => {
	const schemeMatch = query.match(/(?:FROM|INTO|UPDATE)\s+(\w+):\/\//i);
	if (schemeMatch) {
		return schemeMatch[1];
	}
	return null;
};

export const Repl: React.FC<ReplProps> = () => {
	const { exit } = useApp();
	const runtime = useRuntime();
	const [message, setMessage] = useState<string | null>(null);

	const handleSubmit = async (value: string) => {
		const trimmed = value.trim();

		if (!trimmed) {
			return;
		}

		if (trimmed === '/q') {
			setMessage('Bye.');
			exit();
			return;
		}

		try {
			const schema = detectSchema(trimmed);
			if (!schema) {
				setMessage('Could not detect schema from query');
				return;
			}

			const executor = runtime.executors.get(schema);
			if (!executor) {
				setMessage(`No executor for schema: ${schema}`);
				return;
			}

			const result = await executor.query(trimmed);
			const output = `${result.rows.length} rows\n${JSON.stringify(result.rows.slice(0, 5), null, 2)}`;
			setMessage(output);
		} catch (error) {
			setMessage(`Error: ${error instanceof Error ? error.message : String(error)}`);
		}
	};

	return (
		<Box flexDirection="column" padding={1}>
			<Box>
				<Text color="gray">&gt; </Text>
				<HistoryInput onSubmit={handleSubmit} />
			</Box>
			{message && (
				<Box marginTop={1}>
					<Text color="yellow">{message}</Text>
				</Box>
			)}
		</Box>
	);
};

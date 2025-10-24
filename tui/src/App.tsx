import React from 'react';
import { Box, Text } from 'ink';
import { Repl } from '@/Repl.js';

export type AppProps = Record<string, never>;

export const App: React.FC<AppProps> = () => {
	return (
		<Box flexDirection="column" height="100%">
			<Box flexGrow={1} padding={1}>
				<Text color="gray">Pane area (todo)</Text>
			</Box>
			<Box flexShrink={0}>
				<Repl />
			</Box>
		</Box>
	);
};

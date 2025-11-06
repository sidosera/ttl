import React from 'react';
import { Box, Text } from 'ink';
import type { PaneData } from '@/hooks/use_panes.js';

export type PaneProps = {
	pane: PaneData;
	isFocused: boolean;
};

export const Pane: React.FC<PaneProps> = ({ pane, isFocused }) => {
	return (
		<Box
			flexDirection="column"
			borderStyle="single"
			borderColor={isFocused ? 'green' : 'gray'}
			padding={1}
			flexGrow={1}
		>
			<Text>
				Pane: {pane.id} {isFocused && '(focused)'}
			</Text>
			<Text color="gray">Type: {pane.type}</Text>
		</Box>
	);
};

import React from 'react';
import { Box } from 'ink';
import { Pane } from '@/pane.js';
import { usePanes, type PaneData } from '@/hooks/use_panes.js';

export type PaneContainerProps = Record<string, never>;

export const PaneContainer: React.FC<PaneContainerProps> = () => {
	const { panes, focusedPaneId } = usePanes();

	const renderPane = (paneId: string, panes: PaneData[], focusedPaneId: string): React.ReactNode => {
		const pane = panes.find(p => p.id === paneId);
		if (!pane) return null;

		const children = panes.filter(p => p.parent_id === paneId);

		if (children.length === 0) {
			return <Pane key={pane.id} pane={pane} isFocused={pane.id === focusedPaneId} />;
		}

		const firstChild = children[0];
		const direction = firstChild.direction || 'vertical';

		return (
			<Box
				key={pane.id}
				flexDirection={direction === 'vertical' ? 'row' : 'column'}
				flexGrow={1}
			>
				{children.map(child => renderPane(child.id, panes, focusedPaneId))}
			</Box>
		);
	};

	return (
		<Box flexGrow={1}>
			{renderPane('root', panes, focusedPaneId)}
		</Box>
	);
};

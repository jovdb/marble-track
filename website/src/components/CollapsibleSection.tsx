import { createSignal, JSX } from "solid-js";
import styles from "./CollapsibleSection.module.css";

interface CollapsibleSectionProps {
  title: string;
  icon?: JSX.Element;
  children: JSX.Element;
  defaultCollapsed?: boolean;
  headerAction?: JSX.Element;
}

export default function CollapsibleSection(props: CollapsibleSectionProps) {
  const [isCollapsed, setIsCollapsed] = createSignal(props.defaultCollapsed || false);

  const toggleCollapse = () => {
    setIsCollapsed(!isCollapsed());
  };

  return (
    <div class={styles["collapsible-section"]}>
      <button
        class={styles["collapsible-section__header"]}
        onClick={toggleCollapse}
        aria-expanded={!isCollapsed()}
      >
        <div class={styles["collapsible-section__title"]}>
          {props.icon && <span class={styles["collapsible-section__icon"]}>{props.icon}</span>}
          <span>{props.title}</span>
        </div>
        <div class={styles["collapsible-section__header-actions"]}>
          {props.headerAction && !isCollapsed() && (
            <span
              class={styles["collapsible-section__action"]}
              onClick={(e) => e.stopPropagation()}
            >
              {props.headerAction}
            </span>
          )}
          <div
            class={`${styles["collapsible-section__chevron"]} ${
              isCollapsed() ? styles["collapsible-section__chevron--collapsed"] : ""
            }`}
          >
            <svg
              width="20"
              height="20"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
            >
              <path d="M6 9l6 6 6-6" />
            </svg>
          </div>
        </div>
      </button>

      <div
        class={`${styles["collapsible-section__content"]} ${
          isCollapsed() ? styles["collapsible-section__content--collapsed"] : ""
        }`}
      >
        <div class={styles["collapsible-section__inner"]}>{props.children}</div>
      </div>
    </div>
  );
}
